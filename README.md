# Simple Injector — C++20 / Qt 6

Ứng dụng desktop C++20 + Qt 6 Widgets để quản lý danh sách FastFlag và ghi các giá trị
số nguyên/boolean vào `RobloxPlayerBeta.exe` đang chạy (Windows).

## Tính năng

- Thêm flag từ JSON dán vào hoặc nhập từ file `.json` (báo số mục bị bỏ qua).
- Tìm kiếm (có debounce), xóa mục chọn (nút hoặc phím `Delete`), sao chép `Ctrl+C`, xuất file.
- Lưu flag và cài đặt bằng ghi nguyên tử (`QSaveFile`).
- Phát hiện Roblox bằng handle tiến trình (không duyệt lại danh sách tiến trình khi đang kết nối).
- Tải offsets bất đồng bộ, giới hạn kích thước phản hồi, cache cục bộ.
- Tự động Apply đúng một lần mỗi khi gắn vào một tiến trình Roblox mới.
- Báo cáo riêng: đã ghi / thiếu offset / giá trị không hỗ trợ / lỗi ghi.

## Cấu trúc mã nguồn

```
src/
  core/        Logic thuần Qt Core, kiểm thử được
    flag_utils     Chuẩn hóa tên, ép kiểu giá trị, parse offsets, đọc/ghi JSON
    flag_store     Kho flag: nạp, lưu, gộp, xóa, trích tập con
    flag_applier   IMemoryWriter + applyFlags (tên -> offset -> ghi int32)
    offsets_cache  Đọc/ghi cache offsets
    app_storage    Thư mục dữ liệu + di trú từ thư mục "ez" cũ
  platform/    Phần phụ thuộc Windows
    roblox_process Tìm/mở tiến trình, ghi bộ nhớ (cài đặt IMemoryWriter)
    process_monitor Theo dõi định kỳ, phát tín hiệu khi đổi trạng thái
  services/
    offsets_service Tải offsets + cache (mạng)
  ui/          Widgets: main_window, add_flag_dialog, title_bar, style
tests/         Test đơn vị (không cần Roblox, không đụng tiến trình khác)
```

Quy tắc phụ thuộc: `ui` -> `services`/`platform`/`core`; `core` không biết gì về giao diện
hay Windows. Việc ghi bộ nhớ đi qua `IMemoryWriter` nên test dùng writer giả.

## Build cục bộ (Windows)

Yêu cầu: Visual Studio 2022 (Desktop development with C++), CMake ≥ 3.21,
Qt ≥ 6.7 với các module `Core`, `Widgets`, `Network`, `Test`.

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64" -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

File chạy: `build/Release/SimpleInjector.exe`. Có thể hạ yêu cầu Qt khi build thử bằng
`-DSI_QT_MIN_VERSION=6.4` (mã đã được biên dịch và chạy test với Qt 6.4 trên Linux; phần
Windows chỉ biên dịch được trên Windows).

## CI

`.github/workflows/ci.yml` chạy trên push/PR: cài Qt 6.8.3, build, chạy CTest, `windeployqt`,
rồi tải thư mục `Simple-Injector-windows-x64` lên Artifacts. Bản portable không kèm Microsoft
Visual C++ Redistributable (x64) — cài thêm nếu Windows báo thiếu DLL MSVC.

## Dữ liệu

Lưu tại `%LOCALAPPDATA%/SimpleInjector/`: `ffs.json` (flags), `settings.json`, `offsets.json`
(cache). Lần đầu chạy, file khớp tên trong `%LOCALAPPDATA%/ez/` được sao sang nếu chưa có.

## Giới hạn quan trọng

- Chỉ dùng `WriteProcessMemory` có tài liệu với quyền của chính người dùng; không vượt qua bảo vệ
  tiến trình hay anti-cheat của Roblox.
- Offset được hiểu là giá trị 32-bit ghi được, tính từ base của module chính. Ghi thành công ở
  mức hệ điều hành không chứng minh offset còn đúng hay giá trị có ý nghĩa mong muốn.
- Chỉ boolean và số nguyên 32-bit có dấu được Apply; flag kiểu chuỗi chỉ lưu/xuất.
- Offset sai hoặc cũ có thể gây mất ổn định. Chỉ dùng offset bạn được phép dùng và đã tự kiểm chứng.
