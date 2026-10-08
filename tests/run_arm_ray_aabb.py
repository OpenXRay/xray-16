"""Run the production ray/AABB kernel as ARMv7 NEON instructions.

Requires Python unicorn (pip install unicorn) and an Android NDK:
    python3 tests/run_arm_ray_aabb.py --ndk /path/to/android-ndk
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
from unicorn.arm_const import (
    UC_CPU_ARM_CORTEX_A15, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC,
    UC_ARM_REG_R0, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC,
)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--ndk', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
compiler = args.ndk / 'toolchains/llvm/prebuilt/linux-x86_64/bin/clang++'
with tempfile.TemporaryDirectory() as directory:
    binary = Path(directory) / 'ray.bin'
    subprocess.run([
        str(compiler), '--target=armv7a-linux-androideabi21', '-mfpu=neon', '-marm',
        '-O3', '-std=c++17', '-nostdlib', '-fno-exceptions', '-fno-stack-protector',
        '-fno-pic', '-fno-pie', '-Wl,-Ttext=0x10000', '-Wl,-e,test',
        '-Wl,--oformat=binary', '-no-pie', '-DXR_TEST_ARM_ENTRY',
        '-I' + str(root), '-I' + str(root / 'Externals'),
        str(root / 'tests/ray_aabb_simd.cpp'), '-o', str(binary),
    ], check=True)
    machine = Uc(UC_ARCH_ARM, UC_MODE_ARM)
    machine.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A15)
    machine.mem_map(0x10000, 0x100000)
    machine.mem_write(0x10000, binary.read_bytes())
    machine.mem_map(0x300000, 0x10000)
    machine.mem_map(0x400000, 0x10000)
    machine.reg_write(UC_ARM_REG_C1_C0_2, 0xf << 20)
    machine.reg_write(UC_ARM_REG_FPEXC, 1 << 30)

    def call(index):
        machine.reg_write(UC_ARM_REG_R0, index & 0xffffffff)
        machine.reg_write(UC_ARM_REG_SP, 0x40ff00)
        machine.reg_write(UC_ARM_REG_LR, 0x300000)
        machine.emu_start(0x10000, 0x300000, count=1000000)
        assert machine.reg_read(UC_ARM_REG_PC) == 0x300000, 'instruction limit reached'
        return machine.reg_read(UC_ARM_REG_R0)

    count = call(-1)
    assert 0 < count < 100, count
    for index in range(count):
        result = call(index)
        assert result == 0, (index, result)
    print(f'ARMv7 NEON: {count} ray/AABB cases passed')
