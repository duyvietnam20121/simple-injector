# Simple Injector v1.1 — C++20 / Qt 6

Ứng dụng desktop C++20 + Qt 6 Widgets để quản lý danh sách FastFlag và ghi các giá trị
số nguyên/boolean vào `RobloxPlayerBeta.exe` đang chạy (Windows).

## Tính năng

- Thêm flag từ JSON dán vào hoặc nhập từ file `.json` (báo số mục bị bỏ qua).
- Bảng `QAbstractTableModel` + `QSortFilterProxyModel`: sắp xếp theo cột, lọc theo tên/giá trị,
  **sửa giá trị trực tiếp** (nhấp đúp hoặc `F2`), xóa bằng `Delete`, sao chép `Ctrl+C`, xuất file.
- Lưu flag và cài đặt bằng ghi nguyên tử (`QSaveFile`).
- **Cột STATUS cho từng flag**: `● Sẵn sàng`, `✓ Đã áp dụng`, `⚠ Không khớp khi đọc lại`,
  `✗ Không có offset`, `✗ Giá trị không hỗ trợ`, `✗ Lỗi ghi`. Trạng thái được dự đoán ngay từ bảng offsets
  (trước khi Apply), rồi thay bằng kết quả thực tế sau Apply. Rê chuột vào NAME hoặc STATUS để xem lý do
  chi tiết (địa chỉ, giá trị đọc lại...). Có thể sắp xếp theo STATUS (vấn đề lên trước), gõ chữ trạng thái vào
  ô tìm kiếm, hoặc bật "Chỉ hiện flag có vấn đề". Sửa giá trị một flag sẽ xóa kết quả Apply cũ của flag đó;
  mất kết nối hoặc gắn vào tiến trình mới sẽ xóa toàn bộ kết quả cũ.
- **Đọc lại sau khi ghi** (`ReadProcessMemory`) để xác nhận giá trị thực sự đã đổi; báo riêng
  các mục "ghi nhưng đọc lại không khớp" (dấu hiệu offset đã cũ).
- **Đối chiếu phiên bản**: lấy `version-<hex>` của Roblox đang chạy (từ đường dẫn file chạy) và của
  bảng offsets (từ `ClientVersion` trong header). Lệch phiên bản thì cảnh báo, hỏi xác nhận khi Apply
  thủ công, và Auto Apply tự hoãn rồi tải offsets mới trước khi ghi.
- **Làm mới offsets định kỳ** (15 phút), cộng thêm làm mới ngay khi gắn vào Roblox mà đang lệch phiên bản.
- Phát hiện Roblox bằng handle tiến trình (không duyệt lại danh sách tiến trình khi đang kết nối).
- Tự động Apply đúng một lần mỗi khi gắn vào một tiến trình Roblox mới.

## Cấu trúc mã nguồn

```
src/
  core/        Logic không phụ thuộc Widgets/Windows, kiểm thử được
    flag_utils     Chuẩn hóa tên, ép kiểu giá trị, parse offsets, đọc/ghi JSON
    flag_store     Kho flag: nạp, lưu, gộp, sửa, xóa, trích tập con
    flag_table_model Mô hình bảng 3 cột (Tên/Giá trị/Trạng thái) + proxy lọc "chỉ vấn đề" (Qt Core + Gui cho màu)
    flag_applier   IMemoryWriter + applyFlags (ghi int32, đọc lại) + FlagStatus/previewFlag cho từng flag
    offsets_cache  Đọc/ghi cache offsets (kèm phiên bản trong khóa `__meta`)
    roblox_version Phân tích phiên bản từ header/đường dẫn, so khớp
    app_storage    Thư mục dữ liệu + di trú từ thư mục "ez" cũ
  platform/    Phần phụ thuộc Windows
    process_probe  IProcessProbe + ProcessStatus (để test bằng probe giả)
    roblox_process Tìm/mở tiến trình, đọc/ghi bộ nhớ (IMemoryWriter + IProcessProbe)
    process_monitor Theo dõi định kỳ, phát tín hiệu khi đổi trạng thái / vừa gắn
  services/
    offsets_service Tải offsets (thủ công/định kỳ) + cache + phiên bản
  ui/          Widgets: main_window, add_flag_dialog, title_bar, style
tests/         Test đơn vị (không cần Roblox, không đụng tiến trình khác)
  test_offsets_service  Dùng QTcpServer cục bộ giả làm nguồn tải (200, 404, rỗng, quá lớn, treo, tự làm mới)
  test_process_monitor  Dùng IProcessProbe giả (đổi trạng thái, gắn lại, đổi PID, hẹn giờ)
  test_flag_table_model Gồm QAbstractItemModelTester, sửa giá trị, lọc/sắp xếp proxy, 5000 dòng
```

Quy tắc phụ thuộc: `ui` -> `services`/`platform`/`core`; `core` không biết gì về giao diện
hay Windows. Truy cập bộ nhớ đi qua `IMemoryWriter` và tiến trình đi qua `IProcessProbe`,
nên test dùng đối tượng giả.

## Build cục bộ (Windows)

Yêu cầu: Visual Studio 2022 (Desktop development with C++), CMake ≥ 3.21,
Qt ≥ 6.7 với các module `Core`, `Gui`, `Widgets`, `Network`, `Test`.

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

## Phiên bản và icon

- Phiên bản khai báo MỘT chỗ: `project(SimpleInjector VERSION 1.1.0 ...)` trong `CMakeLists.txt`.
  Từ đó CMake sinh ra: số hiển thị `v1.1` trên thanh tiêu đề và tiêu đề cửa sổ, cùng thông tin phiên bản
  trong file `.exe` (Properties > Details). Lên bản mới chỉ cần sửa dòng này.
- Icon nằm ở `resources/icons/`: `app.ico` (nhiều cỡ 16–256 px, gắn vào `.exe` qua `resources/app.rc.in`)
  và `app.png` (nhúng bằng `resources/app.qrc`, dùng cho thanh tiêu đề và icon cửa sổ).
  Muốn đổi icon, thay hai file này (giữ nguyên tên) rồi build lại.

## Dữ liệu

Lưu tại `%LOCALAPPDATA%/SimpleInjector/`: `ffs.json` (flags), `settings.json`, `offsets.json`
(cache). Lần đầu chạy, file khớp tên trong `%LOCALAPPDATA%/ez/` được sao sang nếu chưa có.

## Giới hạn quan trọng

- Chỉ dùng `WriteProcessMemory` có tài liệu với quyền của chính người dùng; không vượt qua bảo vệ
  tiến trình hay anti-cheat của Roblox.
- Offset được hiểu là giá trị 32-bit ghi được, tính từ base của module chính. Đọc lại khớp giá trị
  chỉ chứng minh ô nhớ đó nhận giá trị, không chứng minh đó đúng là flag mong muốn; ngược lại,
  đọc lại không khớp cũng có thể do game tự ghi đè ngay sau đó.
- Việc đối chiếu phiên bản dựa vào tên thư mục `version-<hex>` của file chạy và dòng `ClientVersion`
  trong header offsets. Thiếu một trong hai thì kết quả là "chưa đối chiếu được" (không chặn).
- Chỉ boolean và số nguyên 32-bit có dấu được Apply; flag kiểu chuỗi chỉ lưu/xuất.
- Offset sai hoặc cũ có thể gây mất ổn định. Chỉ dùng offset bạn được phép dùng và đã tự kiểm chứng.
