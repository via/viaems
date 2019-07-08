const Builder = @import("std").build.Builder;
const builtin = @import("builtin");

pub fn build(b: *Builder) void {
    const mode = b.standardReleaseOptions();
    const exe = b.addExecutable("tfi", "main.zig");
    exe.setBuildMode(mode);

    exe.setTarget(builtin.Arch{ .thumb = builtin.Arch.Arm32.v7em }, builtin.Os.freestanding, builtin.Abi.eabihf);

    const args = [_][]const u8{
        "-O3",              "-DSTM32F4",            "-DTICKRATE=4000000",
        "-mfloat-abi=hard", "-mfpu=fpv4-sp-d16",    "-mthumb",
        "-mcpu=cortex-m4",  "-fno-strict-aliasing", "-flto",
    };
    const c_sources = [_][]const u8{
        "decoder.c",
        "util.c",
        "scheduler.c",
        "console.c",
        "calculations.c",
        "config.c",
        "table.c",
        "sensors.c",
        "stats.c",
        "tasks.c",
    };
    exe.setLinkerScriptPath("platforms/stm32f4-discovery.ld");
    exe.addCSourceFile("platforms/stm32f4-discovery.c", args);
    for (c_sources) |source| {
        exe.addCSourceFile(source, args);
    }

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
