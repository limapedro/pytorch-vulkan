import torch
from torch.testing._internal.common_utils import TestCase, run_tests


class TestKaminariRuntime(TestCase):
    def test_device_info(self):
        info = torch.kaminari._C._device_info()
        self.assertTrue(info["available"])
        self.assertTrue(len(info["name"]) > 0)
        self.assertGreater(info["subgroup_size"], 0)
        self.assertGreaterEqual(info["max_workgroup_invocations"], 64)
        self.assertGreater(info["max_shared_memory"], 0)
        self.assertGreater(info["timestamp_valid_bits"], 0)
        self.assertGreaterEqual(info["compute_queue_family"], 0)
        self.assertTrue(info["buffer_device_address"])
        self.assertTrue(info["timeline_semaphore"])
        self.assertTrue(info["shader_float16"])
        self.assertTrue(info["shader_int8"])
        self.assertGreater(len(info["extensions"]), 0)

    def test_synchronize(self):
        torch.kaminari._C._synchronize_vulkan()


if __name__ == "__main__":
    run_tests()
