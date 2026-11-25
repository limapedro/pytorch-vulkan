from torch.testing._internal.common_utils import TestCase, run_tests

import torch
import torch_kaminari  # noqa: F401  (registers the device)


class TestKaminariDevice(TestCase):
    def test_device_parsing(self):
        self.assertEqual(str(torch.device("kaminari:0")), "kaminari:0")
        self.assertEqual(torch.device("kaminari:0").type, "kaminari")

    def test_device_module(self):
        self.assertTrue(torch.kaminari.is_available())
        self.assertEqual(torch.kaminari.device_count(), 1)
        self.assertEqual(torch.kaminari.current_device(), 0)
        self.assertEqual(torch._C._get_privateuse1_backend_name(), "kaminari")

    def test_lazy_init(self):
        torch.kaminari.init()
        self.assertTrue(torch.kaminari.is_initialized())


if __name__ == "__main__":
    run_tests()
