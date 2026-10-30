"""Run the real crash-report writer on a JVM with a four-file session fixture."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class AndroidCrashReportTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which("java"), "JDK required")
    def test_native_and_non_native_exit_reports(self):
        sources = {
            "android/os/Build.java": """package android.os;
                public class Build {
                    public static class VERSION { public static int SDK_INT=36; }
                    public static class VERSION_CODES { public static final int R=30,S=31; }
                }""",
            "android/content/Context.java": """package android.content;
                public class Context {
                    public android.app.ActivityManager manager=new android.app.ActivityManager();
                    public <T> T getSystemService(Class<T> type) { return type.cast(manager); }
                    public String getPackageName() { return "org.openxray.stalker"; }
                }""",
            "android/app/ActivityManager.java": """package android.app;
                public class ActivityManager {
                    public java.util.List<ApplicationExitInfo> exits=new java.util.ArrayList<>();
                    public java.util.List<ApplicationExitInfo> getHistoricalProcessExitReasons(
                        String name,int pid,int count) { return exits; }
                }""",
            "android/app/ApplicationExitInfo.java": """package android.app;
                public class ApplicationExitInfo {
                    public static final int REASON_CRASH_NATIVE=5;
                    public int reason=5; public byte[] trace={1,2,3,4};
                    public String getProcessName() { return "org.openxray.stalker:engine"; }
                    public long getTimestamp() { return 2; }
                    public int getReason() { return reason; }
                    public int getPid() { return 42; }
                    public int getStatus() { return 5; }
                    public long getRss() { return 123; }
                    public String getDescription() { return "crash"; }
                    public java.io.InputStream getTraceInputStream() throws java.io.IOException {
                        return trace==null?null:new java.io.ByteArrayInputStream(trace);
                    }
                }""",
            "android/util/AtomicFile.java": """package android.util;
                public class AtomicFile {
                    private final java.io.File file;
                    public AtomicFile(java.io.File file) { this.file=file; }
                    public java.io.FileOutputStream startWrite() throws java.io.IOException {
                        return new java.io.FileOutputStream(file);
                    }
                    public void finishWrite(java.io.FileOutputStream out) throws java.io.IOException { out.close(); }
                    public void failWrite(java.io.FileOutputStream out) {
                        try { out.close(); } catch(java.io.IOException ignored) {}
                    }
                }""",
            "org/openxray/app/SessionLogs.java": """package org.openxray.app;
                public class SessionLogs {
                    static java.io.File[] files;
                    static java.io.File[] latest(android.content.Context context) { return files; }
                    static long latestStartTime(android.content.Context context) { return 1; }
                }""",
            "org/openxray/app/CrashReportTest.java": """package org.openxray.app;
                import java.nio.file.*;
                import java.util.*;
                public class CrashReportTest {
                    public static void main(String[] args) throws Exception {
                        Path root=Path.of(args[0]);
                        SessionLogs.files=new java.io.File[] {
                            root.resolve("engine.log").toFile(), root.resolve("activity.log").toFile(),
                            root.resolve("exit.txt").toFile(), root.resolve("tombstone.pb").toFile() };
                        Files.writeString(SessionLogs.files[0].toPath(),"engine");
                        Files.writeString(SessionLogs.files[1].toPath(),"activity");
                        android.content.Context context=new android.content.Context();
                        android.app.ApplicationExitInfo exit=new android.app.ApplicationExitInfo();
                        context.manager.exits.add(exit);
                        NativeCrashReport.saveLatest(context);
                        String report=Files.readString(SessionLogs.files[2].toPath());
                        if(!report.contains("pid=42") || !report.contains("tombstone=tombstone.pb"))
                            throw new AssertionError(report);
                        if(!Arrays.equals(Files.readAllBytes(SessionLogs.files[3].toPath()),exit.trace))
                            throw new AssertionError("tombstone bytes missing");
                        NativeCrashReport.saveLatest(context); // repeat is harmless
                        Files.delete(SessionLogs.files[2].toPath());
                        Files.delete(SessionLogs.files[3].toPath());
                        exit.trace=null;
                        NativeCrashReport.saveLatest(context);
                        if(!Files.readString(SessionLogs.files[2].toPath()).contains("unavailable from Android"))
                            throw new AssertionError("missing trace must be reported");
                        Files.delete(SessionLogs.files[2].toPath());
                        exit.reason=1;
                        NativeCrashReport.saveLatest(context);
                        if(!Files.readString(SessionLogs.files[2].toPath()).contains("not a native crash"))
                            throw new AssertionError("non-native exit report missing");
                        if(SessionLogs.files[3].exists()) throw new AssertionError("unexpected tombstone");
                        if(!Files.readString(SessionLogs.files[0].toPath()).equals("engine") ||
                           !Files.readString(SessionLogs.files[1].toPath()).equals("activity"))
                            throw new AssertionError("input logs overwritten");
                    }
                }""",
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for relative, source in sources.items():
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(source)
            actual = ROOT / "android/apk/app/src/main/java/org/openxray/app/NativeCrashReport.java"
            compiler = ["javac"] if shutil.which("javac") else [
                "java", "-m", "jdk.compiler/com.sun.tools.javac.Main"]
            subprocess.run([*compiler, "-d", str(root), str(actual),
                            *map(str, root.rglob("*.java"))], check=True, capture_output=True)
            subprocess.run(["java", "-cp", str(root), "org.openxray.app.CrashReportTest", str(root)],
                           check=True, capture_output=True)


if __name__ == "__main__":
    unittest.main()
