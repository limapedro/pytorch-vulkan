import torch
from torch.testing._internal.common_utils import TestCase, run_tests


class TestKaminariMemory(TestCase):
    def test_empty_copy_roundtrip(self):
        src = torch.tensor([1.0, 2.0, 3.0, 4.0])
        dst = torch.empty(4, device="kaminari", dtype=torch.float32)
        self.assertEqual(dst.device.type, "kaminari")
        dst.copy_(src)
        self.assertEqual(dst.cpu(), src)

    def test_fill_and_zero(self):
        t = torch.empty(4, device="kaminari", dtype=torch.float32)
        t.fill_(2.5)
        self.assertEqual(t.cpu(), torch.full((4,), 2.5))
        t.zero_()
        self.assertEqual(t.cpu(), torch.zeros(4))

    def test_item_and_view(self):
        t = torch.empty((), device="kaminari", dtype=torch.float32)
        t.fill_(7.25)
        self.assertEqual(t.item(), 7.25)
        v = torch.empty(4, device="kaminari").view(2, 2)
        self.assertEqual(v.shape, torch.Size([2, 2]))
        self.assertTrue(v.is_contiguous())

    def test_cpu_fallback_is_counted(self):
        a = torch.full((4,), 1.0, device="kaminari")
        b = torch.full((4,), 2.0, device="kaminari")
        before = torch.kaminari._C._fallback_count()
        c = a + b
        after = torch.kaminari._C._fallback_count()
        self.assertGreater(after, before)
        self.assertEqual(c.cpu(), torch.full((4,), 3.0))


if __name__ == "__main__":
    run_tests()
