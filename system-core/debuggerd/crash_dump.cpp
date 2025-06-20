/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

#include <limits>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <cutils/sockets.h>
#include <log/log.h>
#include <private/android_filesystem_config.h>
#include <procinfo/process.h>

#define ATRACE_TAG ATRACE_TAG_BIONIC
#include <utils/Trace.h>

#include <unwindstack/Regs.h>

#include "libdebuggerd/backtrace.h"
#include "libdebuggerd/tombstone.h"
#include "libdebuggerd/utility.h"

#include "debuggerd/handler.h"
#include "tombstoned/tombstoned.h"

#include "protocol.h"
#include "util.h"

using android::base::unique_fd;
using android::base::StringPrintf;

using unwindstack::Regs;

static bool pid_contains_tid(int pid_proc_fd, pid_t tid) {
  struct stat st;
  std::string task_path = StringPrintf("task/%d", tid);
  return fstatat(pid_proc_fd, task_path.c_str(), &st, 0) == 0;
}

static pid_t get_tracer(pid_t tracee) {
  // Check to see if the thread is being ptraced by another process.
  android::procinfo::ProcessInfo process_info;
  if (android::procinfo::GetProcessInfo(tracee, &process_info)) {
    return process_info.tracer;
  }
  return -1;
}

// Attach to a thread, and verify that it's still a member of the given process
static bool ptrace_seize_thread(int pid_proc_fd, pid_t tid, std::string* error, int flags = 0) {
  if (ptrace(PTRACE_SEIZE, tid, 0, flags) != 0) {
    if (errno == EPERM) {
      pid_t tracer = get_tracer(tid);
      if (tracer != -1) {
        *error = StringPrintf("failed to attach to thread %d, already traced by %d (%s)", tid,
                              tracer, get_process_name(tracer).c_str());
        return false;
      }
    }

    *error = StringPrintf("failed to attach to thread %d: %s", tid, strerror(errno));
    return false;
  }

  // Make sure that the task we attached to is actually part of the pid we're dumping.
  if (!pid_contains_tid(pid_proc_fd, tid)) {
    if (ptrace(PTRACE_DETACH, tid, 0, 0) != 0) {
      PLOG(WARNING) << "failed to detach from thread " << tid;
    }
    *error = StringPrintf("thread %d is not in process", tid);
    return false;
  }

  return true;
}

static bool wait_for_stop(pid_t tid, int* received_signal) {
  while (true) {
    int status;
    pid_t result = waitpid(tid, &status, __WALL);
    if (result != tid) {
      PLOG(ERROR) << "waitpid failed on " << tid << " while detaching";
      return false;
    }

    if (WIFSTOPPED(status)) {
      if (status >> 16 == PTRACE_EVENT_STOP) {
        *received_signal = 0;
      } else {
        *received_signal = WSTOPSIG(status);
      }
      return true;
    }
  }
}

// Interrupt a process and wait for it to be interrupted.
static bool ptrace_interrupt(pid_t tid, int* received_signal) {
  if (ptrace(PTRACE_INTERRUPT, tid, 0, 0) == 0) {
    return wait_for_stop(tid, received_signal);
  }

  PLOG(ERROR) << "failed to interrupt " << tid << " to detach";
  return false;
}

static bool activity_manager_notify(pid_t pid, int signal, const std::string& amfd_data) {
  ATRACE_CALL();
  android::base::unique_fd amfd(socket_local_client(
      "/data/system/ndebugsocket", ANDROID_SOCKET_NAMESPACE_FILESYSTEM, SOCK_STREAM));
  if (amfd.get() == -1) {
    PLOG(ERROR) << "unable to connect to activity manager";
    return false;
  }

  struct timeval tv = {
    .tv_sec = 1,
    .tv_usec = 0,
  };
  if (setsockopt(amfd.get(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
    PLOG(ERROR) << "failed to set send timeout on activity manager socket";
    return false;
  }
  tv.tv_sec = 3;  // 3 seconds on handshake read
  if (setsockopt(amfd.get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
    PLOG(ERROR) << "failed to set receive timeout on activity manager socket";
    return false;
  }

  // Activity Manager protocol: binary 32-bit network-byte-order ints for the
  // pid and signal number, followed by the raw text of the dump, culminating
  // in a zero byte that marks end-of-data.
  uint32_t datum = htonl(pid);
  if (!android::base::WriteFully(amfd, &datum, 4)) {
    PLOG(ERROR) << "AM pid write failed";
    return false;
  }
  datum = htonl(signal);
  if (!android::base::WriteFully(amfd, &datum, 4)) {
    PLOG(ERROR) << "AM signal write failed";
    return false;
  }
  if (!android::base::WriteFully(amfd, amfd_data.c_str(), amfd_data.size() + 1)) {
    PLOG(ERROR) << "AM data write failed";
    return false;
  }

  // 3 sec timeout reading the ack; we're fine if the read fails.
  char ack;
  android::base::ReadFully(amfd, &ack, 1);
  return true;
}

// Globals used by the abort handler.
static pid_t g_target_thread = -1;
static bool g_tombstoned_connected = false;
static bool g_tombstoned_crash_dump_connected = false;
static unique_fd g_tombstoned_socket;
static unique_fd g_tombstoned_crash_dump_socket;
static unique_fd g_output_fd;
static unique_fd g_crash_dump_fd;

// Flag to indicate if crash dump will be generated.
static bool isCrashDumpEnabled = false;

static void DefuseSignalHandlers() {
  // Don't try to dump ourselves.
  struct sigaction action = {};
  action.sa_handler = SIG_DFL;
  debuggerd_register_handlers(&action);

  sigset_t mask;
  sigemptyset(&mask);
  if (sigprocmask(SIG_SETMASK, &mask, nullptr) != 0) {
    PLOG(FATAL) << "failed to set signal mask";
  }
}

static void Initialize(char** argv) {
  android::base::InitLogging(argv);
  android::base::SetAborter([](const char* abort_msg) {
    // If we abort before we get an output fd, contact tombstoned to let any
    // potential listeners know that we failed.
    if (!g_tombstoned_connected) {
      if (!tombstoned_connect(g_target_thread, &g_tombstoned_socket, &g_output_fd,
                              kDebuggerdAnyIntercept)) {
        // We failed to connect, not much we can do.
        LOG(ERROR) << "failed to connected to tombstoned to report failure";
        _exit(1);
      }
    }

    dprintf(g_output_fd.get(), "crash_dump failed to dump process");
    if (g_target_thread != 1) {
      dprintf(g_output_fd.get(), " %d: %s\n", g_target_thread, abort_msg);
    } else {
      dprintf(g_output_fd.get(), ": %s\n", abort_msg);
    }

    _exit(1);
  });
}

static void ParseArgs(int argc, char** argv, pid_t* pseudothread_tid, DebuggerdDumpType* dump_type) {
  if (argc != 4) {
    LOG(FATAL) << "wrong number of args: " << argc << " (expected 4)";
  }

  if (!android::base::ParseInt(argv[1], &g_target_thread, 1, std::numeric_limits<pid_t>::max())) {
    LOG(FATAL) << "invalid target tid: " << argv[1];
  }

  if (!android::base::ParseInt(argv[2], pseudothread_tid, 1, std::numeric_limits<pid_t>::max())) {
    LOG(FATAL) << "invalid pseudothread tid: " << argv[2];
  }

  int dump_type_int;
  if (!android::base::ParseInt(argv[3], &dump_type_int, 0, 1)) {
    LOG(FATAL) << "invalid requested dump type: " << argv[3];
  }
  *dump_type = static_cast<DebuggerdDumpType>(dump_type_int);
}

static void ReadCrashInfo(unique_fd& fd, siginfo_t* siginfo,
                          std::unique_ptr<unwindstack::Regs>* regs,
                          std::unique_ptr<unwindstack::Regs>* crash_dump_regs,
                          uintptr_t* abort_address) {
  std::aligned_storage<sizeof(CrashInfo) + 1, alignof(CrashInfo)>::type buf;
  ssize_t rc = TEMP_FAILURE_RETRY(read(fd.get(), &buf, sizeof(buf)));
  if (rc == -1) {
    PLOG(FATAL) << "failed to read target ucontext";
  } else if (rc != sizeof(CrashInfo)) {
    LOG(FATAL) << "read " << rc << " bytes when reading target crash information, expected "
               << sizeof(CrashInfo);
  }

  CrashInfo* crash_info = reinterpret_cast<CrashInfo*>(&buf);
  if (crash_info->version != 1) {
    LOG(FATAL) << "version mismatch, expected 1, received " << crash_info->version;
  }

  *siginfo = crash_info->siginfo;
  regs->reset(Regs::CreateFromUcontext(Regs::CurrentArch(), &crash_info->ucontext));
  if (crash_dump_regs != NULL && isCrashDumpEnabled) {
    crash_dump_regs->reset(Regs::CreateFromUcontext(Regs::CurrentArch(), &crash_info->ucontext));
  }
  *abort_address = crash_info->abort_msg_address;
}

// Wait for a process to clone and return the child's pid.
// Note: this leaves the parent in PTRACE_EVENT_STOP.
static pid_t wait_for_clone(pid_t pid, bool resume_child) {
  int status;
  pid_t result = TEMP_FAILURE_RETRY(waitpid(pid, &status, __WALL));
  if (result == -1) {
    PLOG(FATAL) << "failed to waitpid";
  }

  if (WIFEXITED(status)) {
    LOG(FATAL) << "traced process exited with status " << WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    LOG(FATAL) << "traced process exited with signal " << WTERMSIG(status);
  } else if (!WIFSTOPPED(status)) {
    LOG(FATAL) << "process didn't stop? (status = " << status << ")";
  }

  if (status >> 8 != (SIGTRAP | (PTRACE_EVENT_CLONE << 8))) {
    LOG(FATAL) << "process didn't stop due to PTRACE_O_TRACECLONE (status = " << status << ")";
  }

  pid_t child;
  if (ptrace(PTRACE_GETEVENTMSG, pid, 0, &child) != 0) {
    PLOG(FATAL) << "failed to get child pid via PTRACE_GETEVENTMSG";
  }

  int stop_signal;
  if (!wait_for_stop(child, &stop_signal)) {
    PLOG(FATAL) << "failed to waitpid on child";
  }

  CHECK_EQ(0, stop_signal);

  if (resume_child) {
    if (ptrace(PTRACE_CONT, child, 0, 0) != 0) {
      PLOG(FATAL) << "failed to resume child (pid = " << child << ")";
    }
  }

  return child;
}

static pid_t wait_for_vm_process(pid_t pseudothread_tid) {
  // The pseudothread will double-fork, we want its grandchild.
  pid_t intermediate = wait_for_clone(pseudothread_tid, true);
  pid_t vm_pid = wait_for_clone(intermediate, false);
  if (ptrace(PTRACE_DETACH, intermediate, 0, 0) != 0) {
    PLOG(FATAL) << "failed to detach from intermediate vm process";
  }

  return vm_pid;
}

int main(int argc, char** argv) {
  DefuseSignalHandlers();

  atrace_begin(ATRACE_TAG, "before reparent");
  pid_t target_process = getppid();

  // Open /proc/`getppid()` before we daemonize.
  std::string target_proc_path = "/proc/" + std::to_string(target_process);
  int target_proc_fd = open(target_proc_path.c_str(), O_DIRECTORY | O_RDONLY);
  if (target_proc_fd == -1) {
    PLOG(FATAL) << "failed to open " << target_proc_path;
  }

  // Make sure getppid() hasn't changed.
  if (getppid() != target_process) {
    LOG(FATAL) << "parent died";
  }
  atrace_end(ATRACE_TAG);


  // Reparent ourselves to init, so that the signal handler can waitpid on the
  // original process to avoid leaving a zombie for non-fatal dumps.
  // Move the input/output pipes off of stdout/stderr, out of paranoia.
  unique_fd output_pipe(dup(STDOUT_FILENO));
  unique_fd input_pipe(dup(STDIN_FILENO));

  unique_fd fork_exit_read, fork_exit_write;
  if (!Pipe(&fork_exit_read, &fork_exit_write)) {
    PLOG(FATAL) << "failed to create pipe";
  }

  bool isSecureBuild = android::base::GetBoolProperty("ro.secure", true);
  bool isDebuggable = android::base::GetBoolProperty("ro.debuggable", false);
  isCrashDumpEnabled = isDebuggable || !isSecureBuild;

  pid_t forkpid = fork();
  if (forkpid == -1) {
    PLOG(FATAL) << "fork failed";
  } else if (forkpid == 0) {
    fork_exit_read.reset();
  } else {
    // We need the pseudothread to live until we get around to verifying the vm pid against it.
    // The last thing it does is block on a waitpid on us, so wait until our child tells us to die.
    fork_exit_write.reset();
    char buf;
    TEMP_FAILURE_RETRY(read(fork_exit_read.get(), &buf, sizeof(buf)));
    _exit(0);
  }

  ATRACE_NAME("after reparent");
  pid_t pseudothread_tid;
  DebuggerdDumpType dump_type;
  uintptr_t abort_address = 0;

  Initialize(argv);
  ParseArgs(argc, argv, &pseudothread_tid, &dump_type);

  // Die if we take too long.
  //
  // Note: processes with many threads and minidebug-info can take a bit to
  //       unwind, do not make this too small. b/62828735
  alarm(30);

  // Get the process name (aka cmdline).
  std::string process_name = get_process_name(g_target_thread);

  // Collect the list of open files.
  OpenFilesList open_files;
  {
    ATRACE_NAME("open files");
    populate_open_files_list(g_target_thread, &open_files);
  }

  // In order to reduce the duration that we pause the process for, we ptrace
  // the threads, fetch their registers and associated information, and then
  // fork a separate process as a snapshot of the process's address space.
  std::set<pid_t> threads;
  if (!android::procinfo::GetProcessTids(g_target_thread, &threads)) {
    PLOG(FATAL) << "failed to get process threads";
  }

  std::map<pid_t, ThreadInfo> thread_info;
  std::map<pid_t, ThreadInfo> crash_dump_thread_info;

  siginfo_t siginfo;
  std::string error;

  {
    ATRACE_NAME("ptrace");
    for (pid_t thread : threads) {
      // Trace the pseudothread separately, so we can use different options.
      if (thread == pseudothread_tid) {
        continue;
      }

      if (!ptrace_seize_thread(target_proc_fd, thread, &error)) {
        bool fatal = thread == g_target_thread;
        LOG(fatal ? FATAL : WARNING) << error;
      }

      ThreadInfo info;
      info.pid = target_process;
      info.tid = thread;
      info.process_name = process_name;
      info.thread_name = get_thread_name(thread);

      ThreadInfo crash_dump_info;
      crash_dump_info.pid = target_process;
      crash_dump_info.tid = thread;
      crash_dump_info.process_name = process_name;
      crash_dump_info.thread_name = get_thread_name(thread);

      if (!ptrace_interrupt(thread, &info.signo)) {
        PLOG(WARNING) << "failed to ptrace interrupt thread " << thread;
        ptrace(PTRACE_DETACH, thread, 0, 0);
        continue;
      }

      if (thread == g_target_thread) {
        if (isCrashDumpEnabled) {
          // Read the thread's registers along with the rest of the crash info out of the pipe.
          ReadCrashInfo(input_pipe, &siginfo, &info.registers,
                                &crash_dump_info.registers,
                                &abort_address);

          crash_dump_info.siginfo = &siginfo;
          crash_dump_info.signo = crash_dump_info.siginfo->si_signo;
        } else {
          // Read the thread's registers along with the rest of the crash info out of the pipe.
          ReadCrashInfo(input_pipe, &siginfo, &info.registers,
                                NULL,
                                &abort_address);
        }
        info.siginfo = &siginfo;
        info.signo = info.siginfo->si_signo;
      } else {
        info.registers.reset(Regs::RemoteGet(thread));
        if (isCrashDumpEnabled) {
          crash_dump_info.registers.reset(Regs::RemoteGet(thread));
        }
        if (!info.registers) {
          PLOG(WARNING) << "failed to fetch registers for thread " << thread;
          ptrace(PTRACE_DETACH, thread, 0, 0);
          continue;
        }
      }

      thread_info[thread] = std::move(info);
      if (isCrashDumpEnabled) {
        crash_dump_thread_info[thread] = std::move(crash_dump_info);
      }
    }
  }

  // Trace the pseudothread with PTRACE_O_TRACECLONE and tell it to fork.
  if (!ptrace_seize_thread(target_proc_fd, pseudothread_tid, &error, PTRACE_O_TRACECLONE)) {
    LOG(FATAL) << "failed to seize pseudothread: " << error;
  }

  if (TEMP_FAILURE_RETRY(write(output_pipe.get(), "\1", 1)) != 1) {
    PLOG(FATAL) << "failed to write to pseudothread";
  }

  pid_t vm_pid = wait_for_vm_process(pseudothread_tid);
  if (ptrace(PTRACE_DETACH, pseudothread_tid, 0, 0) != 0) {
    PLOG(FATAL) << "failed to detach from pseudothread";
  }

  // The pseudothread can die now.
  fork_exit_write.reset();

  // Defer the message until later, for readability.
  bool wait_for_gdb = android::base::GetBoolProperty("debug.debuggerd.wait_for_gdb", false);
  if (siginfo.si_signo == DEBUGGER_SIGNAL) {
    wait_for_gdb = false;
  }

  // Detach from all of our attached threads before resuming.
  for (const auto& [tid, thread] : thread_info) {
    int resume_signal = thread.signo == DEBUGGER_SIGNAL ? 0 : thread.signo;
    if (wait_for_gdb) {
      resume_signal = 0;
      if (tgkill(target_process, tid, SIGSTOP) != 0) {
        PLOG(WARNING) << "failed to send SIGSTOP to " << tid;
      }
    }

    LOG(DEBUG) << "detaching from thread " << tid;
    if (ptrace(PTRACE_DETACH, tid, 0, resume_signal) != 0) {
      PLOG(ERROR) << "failed to detach from thread " << tid;
    }
  }

  // Drop our capabilities now that we've fetched all of the information we need.
  drop_capabilities();

  {
    ATRACE_NAME("tombstoned_connect");
    LOG(INFO) << "obtaining output fd from tombstoned, type: " << dump_type;
    g_tombstoned_connected =
        tombstoned_connect(g_target_thread, &g_tombstoned_socket, &g_output_fd, dump_type);
    if (isCrashDumpEnabled) {
      if (dump_type == kDebuggerdTombstone) {
        g_tombstoned_crash_dump_connected =
            tombstoned_connect(g_target_thread, &g_tombstoned_crash_dump_socket, &g_crash_dump_fd, kDebuggerdCrashDump);
      }
    }
  }

  if (g_tombstoned_connected) {
    if (TEMP_FAILURE_RETRY(dup2(g_output_fd.get(), STDOUT_FILENO)) == -1) {
      PLOG(ERROR) << "failed to dup2 output fd (" << g_output_fd.get() << ") to STDOUT_FILENO";
    }
  } else {
    unique_fd devnull(TEMP_FAILURE_RETRY(open("/dev/null", O_RDWR)));
    TEMP_FAILURE_RETRY(dup2(devnull.get(), STDOUT_FILENO));
    g_output_fd = std::move(devnull);
  }

  if (isCrashDumpEnabled) {
    if (dump_type == kDebuggerdTombstone) {
      if (g_tombstoned_crash_dump_connected) {
        if (TEMP_FAILURE_RETRY(dup2(g_crash_dump_fd.get(), STDOUT_FILENO)) == -1) {
          PLOG(ERROR) << "failed to dup2 crash dump fd (" << g_crash_dump_fd.get() << ") to STDOUT_FILENO";
        }
      } else {
        unique_fd devnull(TEMP_FAILURE_RETRY(open("/dev/null", O_RDWR)));
        TEMP_FAILURE_RETRY(dup2(devnull.get(), STDOUT_FILENO));
        g_crash_dump_fd = std::move(devnull);
      }
    }
  }

  LOG(INFO) << "performing dump of process " << target_process << " (target tid = " << g_target_thread
            << ")";

  int signo = siginfo.si_signo;
  bool fatal_signal = signo != DEBUGGER_SIGNAL;
  bool backtrace = false;

  // si_value is special when used with DEBUGGER_SIGNAL.
  //   0: dump tombstone
  //   1: dump backtrace
  if (!fatal_signal) {
    int si_val = siginfo.si_value.sival_int;
    if (si_val == 0) {
      backtrace = false;
    } else if (si_val == 1) {
      backtrace = true;
    } else {
      LOG(WARNING) << "unknown si_value value " << si_val;
    }
  }

  // TODO: Use seccomp to lock ourselves down.
  std::unique_ptr<BacktraceMap> map(BacktraceMap::Create(vm_pid, false));
  if (!map) {
    LOG(FATAL) << "failed to create backtrace map";
  }

  std::shared_ptr<unwindstack::Memory> process_memory = map->GetProcessMemory();
  if (!process_memory) {
    LOG(FATAL) << "failed to get unwindstack::Memory handle";
  }

  std::string amfd_data;
  if (backtrace) {
    ATRACE_NAME("dump_backtrace");
    dump_backtrace(std::move(g_output_fd), map.get(), thread_info, g_target_thread);
  } else {
    ATRACE_NAME("engrave_tombstone");
    engrave_tombstone(std::move(g_output_fd), map.get(), process_memory.get(), thread_info,
                      g_target_thread, abort_address, &open_files, &amfd_data);
    if (isCrashDumpEnabled) {
      if (dump_type == kDebuggerdTombstone) {
        capture_crash_dump(std::move(g_crash_dump_fd), map.get(), process_memory.get(), g_target_thread, crash_dump_thread_info);
      }
    }
  }

  if (fatal_signal) {
    // Don't try to notify ActivityManager if it just crashed, or we might hang until timeout.
    if (thread_info[target_process].thread_name != "system_server") {
      activity_manager_notify(target_process, signo, amfd_data);
    }
  }

  if (wait_for_gdb) {
    // Use ALOGI to line up with output from engrave_tombstone.
    ALOGI(
        "***********************************************************\n"
        "* Process %d has been suspended while crashing.\n"
        "* To attach gdbserver and start gdb, run this on the host:\n"
        "*\n"
        "*     gdbclient.py -p %d\n"
        "*\n"
        "***********************************************************",
        target_process, target_process);
  }

  // Close stdout before we notify tombstoned of completion.
  close(STDOUT_FILENO);
  if (g_tombstoned_connected && !tombstoned_notify_completion(g_tombstoned_socket.get())) {
    LOG(ERROR) << "failed to notify tombstoned of completion";
  }

  if (isCrashDumpEnabled) {
    if (dump_type == kDebuggerdTombstone) {
      if (g_tombstoned_crash_dump_connected && !tombstoned_notify_completion(g_tombstoned_crash_dump_socket.get())) {
        LOG(ERROR) << "failed to notify tombstoned of completion";
      }
    }
  }

  return 0;
}
