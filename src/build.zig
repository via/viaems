const Builder = @import("std").build.Builder;
const builtin = @import("builtin");

pub fn build(b: *Builder) void {
    const mode = b.standardReleaseOptions();
    const exe = b.addExecutable("tfi", "main.zig");
    exe.setBuildMode(mode);

    exe.setTarget(builtin.Arch{.thumb = builtin.Arch.Arm32.v7em},
    builtin.Os.freestanding, builtin.Abi.eabihf);

    const args = [_][]const u8{"-O3", "-DSTM32F4", "-DTICKRATE=4000000",
    "-mfloat-abi=hard", "-mfpu=fpv4-sp-d16", "-mthumb", "-mcpu=cortex-m4",
    "-fno-strict-aliasing", "-flto"};
    exe.setLinkerScriptPath("platforms/stm32f4-discovery.ld");
    exe.addCSourceFile("platforms/stm32f4-discovery.c", args);
    exe.addCSourceFile("decoder.c", args);
    exe.addCSourceFile("util.c", args);
    exe.addCSourceFile("scheduler.c", args);
    exe.addCSourceFile("console.c", args);
    exe.addCSourceFile("calculations.c", args);
    exe.addCSourceFile("config.c", args);
    exe.addCSourceFile("table.c", args);
    exe.addCSourceFile("sensors.c", args);
    exe.addCSourceFile("stats.c", args);
    exe.addCSourceFile("tasks.c", args);
    exe.addCSourceFile("tfi.c", args);
    exe.addIncludeDir(".");
    exe.addIncludeDir("../libopencm3/include");
    exe.addIncludeDir("/usr/lib/arm-none-eabi/include");
    exe.addLibPath("../libopencm3/lib");
    exe.linkSystemLibrary("opencm3_stm32f4");
    exe.addObjectFile("/usr/lib/arm-none-eabi/lib/thumb/v7e-m/libc.a");
    exe.addObjectFile("/usr/lib/arm-none-eabi/lib/thumb/v7e-m/libm.a");
    exe.addObjectFile("/usr/lib/arm-none-eabi/lib/thumb/v7e-m/libnosys.a");
    exe.addObjectFile("/usr/lib/gcc/arm-none-eabi/7.3.1/thumb/v7e-m/fpv4-sp/hard/libgcc.a");
    exe.setOutputDir("output");

    const run_cmd = exe.run();

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    b.default_step.dependOn(&exe.step);
    b.installArtifact(exe);
}
