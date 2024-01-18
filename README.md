 Nguyên lý hoạt động
 =
- Sử dụng cảm biến lưu lượng đo lưu lượng chất lỏng chảy qua và ngõ ra cho tín hiệu ml/xung. (Quy ước 2.25ml/xung).
- Vi điều khiển sử dụng ngắt để đọc tín hiệu xung và thực hiện cộng dồn cho ra số lít đã bơm và số tiền phải trả.
- Bàn phím giao tiếp cho phép nhập số tiền hoặc số lít muốn bơm, cài đặt giá trị đơn giá/lít, reset và các phím với số tiền bơm mặc định.
- Màn hình LCD 20x4 hiển thị các giá trị SỐ TIỀN phải trả, SỐ LÍT đã bơm, CHỌN GIÁ tiền muốn bơm hoặc số lít, ĐƠN GIÁ trên một lít.
- Một công tắt cho phép bắt đầu bơm (mở sau mỗi lần bơm xong).
- Relay tự động ngắt khi bơm đủ số tiền hoặc lít đã chọn và phải reset lại sau mỗi lần bơm.
- Module ESP8266-01 kết nối wifi dùng tập lệnh AT gửi giá trị SỐ TIỀN và SỐ LÍT lên trang web ThingSpeak.

Các thành phần mô phỏng trong Proteus
=
- Tín hiệu xung mô phỏng cho cảm biến lưu lượng.
- Bàn phím Keypad 4x4 mô phỏng cho bàn phím nhập.
- Màn hình LCD 20x4.
- Motor mô phỏng cho việc bơm xăng.
- 1 Relay điều khiển motor và 1 relay điều khiển tín hiệu xung vào VĐK (vì ngắt motor là ngưng bơm đồng nghĩa không còn xung vào VĐK).
- 1 công tắc đôi cho phép bắt đầu bơm.
- Dùng cổng COMPIM kết nối với module ESP8266-01.

Chức năng các phím trên Keypad 4x4
=
- Các phím số từ 0-9 dùng để nhập số.
- Phím ON/C Reset SỐ TIỀN, SỐ LÍT và Relay sau mỗi lần bơm.
- Phím = cài đặt đơn giá. 
- Phím + xóa đơn giá hiện tại.  
- Phím % là phím mặc định bơm 100k.
- Phím X là phím mặc định bơm 50k.
- Phím - là phím mặc định bơm 20k.
