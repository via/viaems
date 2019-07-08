const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.build.Builder) void {
    const mode = b.standardReleaseOptions();
    const exe = b.addExecutable("tfi", "main.zig");
    exe.setBuildMode(mode);

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

    //    platform_stm32f407(exe, c_sources);
    platform_hosted(exe, c_sources);

    exe.addIncludeDir(".");
    exe.setOutputDir("output");

    b.default_step.dependOn(&exe.step);
    b.installArtifact(exe);
}

fn platform_hosted(exe: *std.build.LibExeObjStep, c_sources: []const []const u8) void {
    const c_args = [_][]const u8{"-DTICKRATE=1000000",
    "-DSUPPORTS_POSIX_TIMERS", "-D_POSIX_C_SOURCE=199309L", "-D_GNU_SOURCE"};

    exe.setTarget(builtin.Arch.x86_64, builtin.Os.linux, builtin.Abi.gnu);
    exe.addCSourceFile("platforms/hosted.c", c_args);
    for (c_sources) |source| {
        exe.addCSourceFile(source, c_args);
    }

    exe.linkSystemLibrary("c");
    exe.linkSystemLibrary("m");
    exe.linkSystemLibrary("rt");
}

fn platform_stm32f407(exe: *std.build.LibExeObjStep, c_sources: []const []const u8) void {
    const c_args = [_][]const u8{
        "-O3",              "-DSTM32F4",            "-DTICKRATE=4000000",
        "-mfloat-abi=hard", "-mfpu=fpv4-sp-d16",    "-mthumb",
        "-mcpu=cortex-m4",  "-fno-strict-aliasing", "-flto",
    };
    exe.setTarget(builtin.Arch{ .thumb = builtin.Arch.Arm32.v7em }, builtin.Os.freestanding, builtin.Abi.eabihf);
    exe.setLinkerScriptPath("platforms/stm32f4-discovery.ld");

    exe.addCSourceFile("platforms/stm32f4-discovery.c", c_args);
    for (c_sources) |source| {
        exe.addCSourceFile(source, c_args);
    }

    exe.addIncludeDir("../libopencm3/include");
    exe.addIncludeDir("/usr/lib/arm-none-eabi/include");
    exe.addLibPath("../libopencm3/lib");
    exe.linkSystemLibrary("opencm3_stm32f4");
    exe.addObjectFile("/usr/lib/arm-none-eabi/lib/thumb/v7e-m/libc.a");
    exe.addObjectFile("/usr/lib/arm-none-eabi/lib/thumb/v7e-m/libm.a");
    exe.addObjectFile("/usr/lib/arm-none-eabi/lib/thumb/v7e-m/libnosys.a");
    exe.addObjectFile("/usr/lib/gcc/arm-none-eabi/7.3.1/thumb/v7e-m/fpv4-sp/hard/libgcc.a");
}
