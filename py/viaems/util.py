TICKRATE = 4000000.0


def ticks_for_rpm_degrees(rpm, degrees):
    ticks_per_degree = (4000000 / 6.0) / rpm
    return int(degrees * ticks_per_degree)


def degrees_for_tick_rpm(ticks, rpm):
    ticks_per_degree = (4000000 / 6.0) / rpm
    return ticks / ticks_per_degree


def ms_ticks(ms):
    return 4000 * ms


def clamp_angle(angle):
    if angle >= 720:
        angle -= 720
    if angle < 0:
        angle += 720
    return angle
