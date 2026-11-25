import os
import subprocess
import sys
import sysconfig

from setuptools import find_packages, setup

BASE_DIR = os.path.dirname(os.path.realpath(__file__))
SKIP_BUILD = any(arg in {"clean", "dist_info", "egg_info"} for arg in sys.argv)


def torch_cmake_prefix():
    return os.path.join(sysconfig.get_paths()["purelib"], "torch")


def pybind11_cmake_prefix():
    code = "import pybind11; print(pybind11.get_cmake_dir())"
    return subprocess.check_output([sys.executable, "-c", code], text=True).strip()


def build_extension():
    build_dir = os.path.join(BASE_DIR, "build_cmake")
    os.makedirs(build_dir, exist_ok=True)
    prefix = os.path.join(BASE_DIR, "torch_kaminari")
    cmake_args = [
        "-S",
        BASE_DIR,
        "-B",
        build_dir,
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=" + prefix,
        "-DCMAKE_PREFIX_PATH=" + torch_cmake_prefix() + ";" + pybind11_cmake_prefix(),
    ]
    subprocess.check_call(["cmake"] + cmake_args)
    subprocess.check_call(
        ["cmake", "--build", build_dir, "--config", "Release", "--target", "install"]
    )


if not SKIP_BUILD:
    build_extension()

setup(
    name="torch-kaminari",
    version="0.1.0",
    description="Kaminari: a Vulkan compute backend for PyTorch",
    packages=find_packages(),
    package_data={"torch_kaminari": ["*.pyd", "*.so", "*.dll"]},
    entry_points={"torch.backends": ["torch_kaminari = torch_kaminari:_autoload"]},
)
