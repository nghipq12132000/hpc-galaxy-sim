import os

dest_file = r"C:\Users\ADMIN\.gemini\antigravity\brain\8d221dba-b004-4e02-b94c-4dce3cee80dc\HPC_Theory_Master_Guide.md"

guide_content = """# Cẩm Nang Lý Thuyết Tính Toán Hiệu Năng Cao (HPC)

Tài liệu này tổng hợp và hệ thống hóa toàn bộ kiến thức lý thuyết từ các slide môn học **Tính toán hiệu năng cao**, bao gồm các chủ đề từ lập trình đa luồng (shared-memory parallelism), phân tích hiệu năng (Performance Analysis), thuật toán song song nâng cao, lập trình truyền thông điệp (MPI), đến các mô hình xử lý dữ liệu lớn (Hadoop HDFS & MapReduce).

---

## PHẦN I: SONG SONG HÓA TRÊN BỘ NHỚ CHIA SẺ & ĐA LUỒNG (SHARED-MEMORY & MULTITHREADING)

### 1. Giới thiệu về Tính toán Song song (Parallel Computing)
*   **Khái niệm cơ bản**:
    *   *Tính toán tuần tự (Serial Computing)*: Một bài toán được chia thành chuỗi các chỉ thị thực hiện tuần tự trên một CPU duy nhất.
    *   *Tính toán song song (Parallel Computing)*: Sử dụng đồng thời nhiều tài nguyên tính toán để giải quyết một bài toán. Bài toán được chia nhỏ thành các phần độc lập có thể giải quyết đồng thời.
*   **Phân loại kiến trúc phần cứng (Flynn's Taxonomy)**:
    1.  **SISD** (Single Instruction, Single Data): Máy tính tuần tự cổ điển.
    2.  **SIMD** (Single Instruction, Multiple Data): Một lệnh áp dụng đồng thời trên nhiều luồng dữ liệu (ví dụ: GPU, chỉ thị vector SSE/AVX).
    3.  **MISD** (Multiple Instruction, Single Data): Nhiều chỉ thị trên một luồng dữ liệu (ít phổ biến, ứng dụng trong hệ thống chịu lỗi).
    4.  **MIMD** (Multiple Instruction, Multiple Data): Nhiều bộ xử lý thực hiện các dòng lệnh độc lập trên các luồng dữ liệu khác nhau (phổ biến nhất ở các hệ thống đa nhân hiện đại).
*   **Kiến trúc bộ nhớ**:
    *   *Shared-Memory (Bộ nhớ chia sẻ)*: Các nhân CPU truy cập chung vào một không gian địa chỉ bộ nhớ vật lý. Phối hợp thông qua việc đọc/ghi các biến chia sẻ.
    *   *Distributed-Memory (Bộ nhớ phân tán)*: Mỗi bộ xử lý có bộ nhớ cục bộ riêng. Phối hợp và truyền dữ liệu thông qua truyền thông điệp qua mạng (Message Passing).
    *   *Hybrid*: Kết hợp cả hai (ví dụ: các node mạng có bộ nhớ chia sẻ đa nhân, liên kết với nhau qua mạng phân tán).

### 2. Tiến trình (Processes) và Luồng (Threads) trong Java
*   **Tiến trình (Process)**: Một thực thể độc lập của hệ điều hành, sở hữu không gian địa chỉ riêng, tài nguyên riêng. Việc chuyển ngữ cảnh (context switch) giữa các tiến trình tốn kém.
*   **Luồng (Thread)**: Là đơn vị thực thi nhỏ nhất bên trong một tiến trình ("lightweight process"). Các luồng chung tiến trình chia sẻ không gian bộ nhớ (heap, static data, code) nhưng có ngăn xếp (stack) và PC (program counter) riêng.
*   **Đa luồng trong Java**:
    *   *Cách khởi tạo*:
        1.  Kế thừa lớp `Thread`: Ghi đè phương thức `run()`, kích hoạt bằng `.start()`.
        2.  Triển khai giao diện `Runnable`: Truyền đối tượng `Runnable` vào constructor của `Thread`. (Khuyên dùng vì giúp tránh giới hạn đơn kế thừa của Java).
    *   *Quản lý vòng đời*:
        *   `start()`: Đăng ký luồng với bộ lập lịch (scheduler), luồng chuyển sang trạng thái *Runnable*.
        *   `join()`: Luồng hiện tại tạm dừng và đợi luồng mục tiêu kết thúc hoàn toàn mới chạy tiếp.
        *   `sleep(ms)`: Luồng tạm dừng thực thi trong khoảng thời gian chỉ định mà không giải phóng khóa (lock).

### 3. Phân tích Hiệu năng (Performance of Parallel Programs)
*   **Work và Span (Mô hình đồ thị DAG)**:
    *   Một chương trình song song Fork-Join có thể biểu diễn bằng một Đồ thị có hướng không chu trình (DAG).
    *   **Work ($T_1$)**: Tổng thời gian thực hiện trên 1 bộ xử lý (tổng số node trong DAG).
    *   **Span ($T_\\infty$)**: Thời gian thực thi trên vô số bộ xử lý (chiều dài đường đi dài nhất/đường găng trong DAG - critical path).
    *   **Parallelism (Độ song song tối đa)**: Tỷ số $\\frac{T_1}{T_\\infty}$. Cho biết giới hạn trên của tốc độ tăng tốc khi tăng số bộ xử lý.
*   **Tốc độ tăng tốc (Speedup) & Hiệu suất (Efficiency)**:
    *   $Speedup(P) = \\frac{T_1}{T_P}$ (Lý tưởng nhất là $Speedup(P) = P$ - tuyến tính).
    *   $Efficiency(P) = \\frac{Speedup(P)}{P} = \\frac{T_1}{P \\cdot T_P}$ (Thường $< 1$ do chi phí đồng bộ hóa, phân tán dữ liệu).
*   **Định luật Amdahl (Amdahl's Law) - Giới hạn tuần tự**:
    *   Giả sử chương trình có phần tuần tự là $s$ và phần song song hóa hoàn hảo là $p$ (với $s + p = 1$).
    *   $$Speedup(P) = \\frac{1}{s + \\frac{p}{P}}$$
    *   Khi $P \\to \\infty$, $Speedup \\to \\frac{1}{s}$.
    *   *Ý nghĩa*: Nếu chương trình có 5% tuần tự ($s = 0.05$), thì dù có vô số bộ xử lý, Speedup tối đa cũng chỉ đạt $20$. Định luật Amdahl từng khiến giới nghiên cứu bi quan về tương lai của máy tính song song.
*   **Định luật Gustafson (Gustafson's Law) - Góc nhìn thực tế**:
    *   Chỉ ra rằng trong thực tế, khi tăng số lượng bộ xử lý, người ta thường tăng kích thước bài toán (Problem Size) thay vì giữ nguyên bài toán cũ.
    *   Định luật Gustafson định nghĩa tốc độ tăng tốc theo kích thước bài toán thay đổi (Scaled Speedup):
        $$Speedup_{scaled}(P) = P + (1 - P) \\cdot s$$
    *   *Ý nghĩa*: Phần tuần tự $s$ thường giảm đi khi kích thước dữ liệu tăng lên, cho phép mở rộng hệ thống hiệu quả lên hàng ngàn nhân.
*   **Mở rộng (Scaling)**:
    *   *Strong Scaling*: Giữ nguyên tổng kích thước bài toán, đo thời gian chạy khi tăng số lượng bộ xử lý.
    *   *Weak Scaling*: Giữ nguyên kích thước bài toán trên mỗi bộ xử lý (tổng kích thước bài toán tăng tỉ lệ thuận với số bộ xử lý), đo thời gian chạy.

### 4. Thuật toán Song song Cơ bản (Parallel Algorithms)
*   **Fork-Join Framework**:
    *   Cơ chế lập lịch tự động phân chia công việc theo dạng cây nhị phân (Divide-and-Conquer).
    *   Sử dụng cơ chế **Work-Stealing**: Các luồng rảnh rỗi sẽ "trộm" công việc từ cuối hàng đợi của các luồng đang bận rộn, giúp cân bằng tải tối ưu.
    *   *RecursiveAction* (không trả về kết quả) và *RecursiveTask<V>* (có trả về kết quả).
*   **Bản đồ & Thu gọn (Maps & Reductions)**:
    *   *Map*: Áp dụng một hàm độc lập lên từng phần tử của mảng. Độ phức tạp: Work $O(n)$, Span $O(1)$ (hoặc $O(\\log n)$ do chi phí fork).
    *   *Reduction*: Kết hợp các phần tử bằng toán tử có tính kết hợp (cộng, nhân, max, min). Độ phức tạp: Work $O(n)$, Span $O(\\log n)$.
*   **Thuật toán Prefix-Sum (Quét song song - Scan)**:
    *   Bài toán: Cho mảng $A$, tính mảng đầu ra $B$ sao cho $B[i] = \\sum_{j=0}^{i} A[j]$.
    *   Thuật toán song song thực hiện qua 2 lượt:
        1.  **Lượt lên (Up-pass)**: Xây dựng cây nhị phân tích lũy tổng các khoảng (tương tự reduction). Tính toán giá trị tổng của các node từ dưới lên.
        2.  **Lượt xuống (Down-pass)**: Đi từ gốc xuống, truyền giá trị tích lũy từ bên trái (`fromLeft`). Node con bên trái nhận `fromLeft` của cha. Node con bên phải nhận `fromLeft` cộng với giá trị của con bên trái đã lưu ở lượt Up-pass.
    *   Độ phức tạp: Work $O(n)$, Span $O(\\log n)$. Cho phép tăng tốc lũy thừa.
*   **Bộ lọc song song (Pack/Filter)**:
    *   Bài toán: Giữ lại các phần tử thỏa mãn điều kiện $f(x)$ mà vẫn bảo toàn thứ tự.
    *   Các bước thực hiện:
        1.  Map tạo mảng bit biểu diễn kết quả điều kiện: `[1, 0, 1, 1]`.
        2.  Chạy thuật toán Prefix-Sum trên mảng bit để tính toán chỉ số vị trí đích trong mảng kết quả: `[1, 1, 2, 3]`.
        3.  Map ghi đè các phần tử thỏa mãn vào mảng đích dựa trên vị trí đã tính.
*   **Sắp xếp song song (Parallel Sorting)**:
    *   *Quicksort song song tuần tự*: Gọi đệ tử quy song song cho 2 nửa phân hoạch. Span giảm từ $O(n \\log n)$ xuống $O(n)$ do bước phân hoạch vẫn chạy tuần tự $O(n)$.
    *   *Quicksort song song cải tiến*: Song song hóa cả bước phân hoạch bằng thuật toán Pack song song ($O(\\log n)$ span). Span của toàn bộ thuật toán giảm xuống $O(\\log^2 n)$, hiệu năng tăng vượt trội.

---

## PHẦN II: ĐỒNG BỘ HÓA LUỒNG & AN TOÀN LUỒNG (SYNCHRONIZATION & THREAD SAFETY)

### 1. Tranh chấp dữ liệu (Race Conditions) & Đoạn găng (Critical Sections)
*   **Race Condition**: Xảy ra khi nhiều luồng truy cập và sửa đổi đồng thời dữ liệu dùng chung, và kết quả cuối cùng phụ thuộc vào trật tự thực thi xen kẽ (interleaving) của các luồng.
*   **Critical Section**: Đoạn mã chứa các thao tác truy cập dữ liệu dùng chung cần được đảm bảo loại trừ tương hỗ (Mutual Exclusion). Tại một thời điểm, chỉ một luồng được phép thực thi trong đoạn găng.

### 2. Loại trừ Tương hỗ & Đồng bộ hóa trong Java
*   **Khóa nội tại (Intrinsic Lock / Monitor Lock)**:
    *   Mỗi đối tượng Java đều có một khóa ẩn đi kèm.
    *   Sử dụng từ khóa `synchronized`:
        *   *Khối synchronized*: `synchronized(lockObject) { ... }` - Cho phép chỉ khóa phạm vi mã nhỏ (tối ưu hiệu năng).
        *   *Phương thức synchronized*: Khóa toàn bộ thân phương thức bằng cách sử dụng `this` làm khóa.
    *   **Khóa tái nhập (Reentrant Lock)**: Java lock cho phép luồng đang giữ khóa tiếp tục đi vào các khối mã khác yêu cầu cùng một khóa đó mà không bị tự khóa chính mình (deadlock).
*   **Biến nguyên tử (Atomic Variables)**:
    *   Gói `java.util.concurrent.atomic` cung cấp các lớp như `AtomicInteger`, `AtomicLong`.
    *   Sử dụng kỹ thuật phần cứng CAS (Compare-And-Swap) không cần khóa (lock-free) để thực hiện các phép toán đọc-sửa-ghi nguyên tử, giảm thiểu chi phí nghẽn luồng.

### 3. Thiết kế Lớp An toàn Luồng (Thread Safety)
*   Một lớp là **An toàn Luồng (Thread-safe)** nếu nó hoạt động đúng khi nhiều luồng gọi các phương thức của nó đồng thời, mà không cần thêm bất kỳ sự đồng bộ hóa nào ở phía gọi.
*   **Java Monitor Pattern**:
    *   Bao bọc tất cả trạng thái có thể thay đổi của đối tượng.
    *   Bảo vệ trạng thái đó bằng một khóa duy nhất của chính đối tượng.
*   **Thao tác phức hợp (Compound Actions)**:
    *   Các mẫu lỗi phổ biến: "Check-then-act" (Kiểm tra rồi mới hành động - ví dụ: Singleton lười khởi tạo không đồng bộ) và "Read-modify-write" (Đọc-sửa-ghi - ví dụ: `count++`). Các thao tác này phải được thực hiện nguyên tử dưới một khóa chung.

### 4. Các Vấn đề Liveness: Deadlock & Phối hợp Luồng
*   **Bế tắc (Deadlock)**:
    *   Tình huống hai hoặc nhiều luồng bị phong tỏa vĩnh viễn, mỗi luồng chờ đợi tài nguyên mà luồng kia đang nắm giữ.
    *   **4 điều kiện cần để xảy ra Deadlock (Coffman)**:
        1.  *Mutual Exclusion*: Tài nguyên không thể chia sẻ.
        2.  *Hold and Wait*: Luồng giữ tài nguyên đã cấp và đang đợi tài nguyên mới.
        3.  *No Preemption*: Không thể thu hồi tài nguyên từ luồng đang giữ.
        4.  *Circular Wait*: Tồn tại chu kỳ chờ đợi vòng tròn giữa các luồng.
    *   *Giải pháp*: Phá vỡ chu kỳ bằng cách sắp xếp thứ tự lấy khóa cố định (Lock Ordering) hoặc sử dụng thời gian chờ (lock timeout).
*   **Đồng bộ hóa Điều kiện (Condition Variables)**:
    *   `wait()`: Giải phóng khóa hiện tại và đưa luồng vào trạng thái chờ cho đến khi được đánh thức.
    *   `notify()`: Đánh thức một luồng ngẫu nhiên đang chờ trên khóa.
    *   `notifyAll()`: Đánh thức tất cả các luồng đang chờ trên khóa.
    *   **Quy tắc quan trọng**:
        1.  Luôn gọi `wait()` bên trong vòng lặp `while(điều_kiện_chờ)` thay vì lệnh `if`, để tránh hiện tượng đánh thức giả (spurious wakeup).
        2.  Ưu tiên dùng `notifyAll()` hơn `notify()` để tránh lỗi mất tín hiệu (missed signals) khi có nhiều loại điều kiện chờ khác nhau trên cùng một khóa.
*   **Các Công cụ Đồng bộ hóa trong Java (Synchronizers)**:
    *   **BlockingQueue**: Hàng đợi chặn dùng cho mẫu thiết kế Nhà sản xuất - Người tiêu dùng (Producer-Consumer). Phương thức `put()` chặn nếu hàng đợi đầy, `take()` chặn nếu hàng đợi rỗng. Các lớp triển khai phổ biến: `ArrayBlockingQueue`, `LinkedBlockingQueue`, `SynchronousQueue`.
    *   **CountDownLatch**: Bộ đếm lùi dùng để chặn một hoặc nhiều luồng cho đến khi một số sự kiện hoàn thành (đếm về 0). Không thể tái sử dụng.
    *   **Semaphore**: Quản lý một tập hợp các giấy phép (permits). Dùng để giới hạn số luồng truy cập đồng thời vào tài nguyên.
    *   **CyclicBarrier**: Chặn một nhóm luồng tại một điểm hẹn (rendezvous) cho đến khi toàn bộ nhóm đều đến nơi. Thường dùng trong các thuật toán mô phỏng lặp song song (như Trò chơi cuộc sống của Conway). Có thể tái sử dụng.

---

## PHẦN III: LẬP TRÌNH SONG SONG PHÂN TÁN VỚI MPI (MESSAGE PASSING INTERFACE)

### 1. Kiến trúc Bộ nhớ Phân tán & Tổng quan MPI
*   **Đặc điểm**: Mỗi bộ xử lý (process) chạy trong một không gian bộ nhớ riêng biệt. Không có dữ liệu chung trực tiếp. Các tiến trình giao tiếp bằng cách gửi và nhận các thông điệp qua mạng liên kết.
*   **MPI**: Là đặc tả thư viện chuẩn cho truyền thông điệp, độc lập với nhà cung cấp, hỗ trợ C, C++, Fortran.
*   **Mô hình SPMD (Single Program Multiple Data)**: Bản sao của cùng một chương trình được chạy trên tất cả các tiến trình, nhưng mỗi tiến trình thực thi các nhánh lệnh khác nhau dựa trên mã định danh (rank) của nó.

### 2. Các hàm MPI Cơ bản & Quản lý Tiến trình
*   `MPI_Init(&argc, &argv)`: Khởi tạo môi trường MPI. Phải là hàm MPI đầu tiên được gọi.
*   `MPI_Finalize()`: Dọn dẹp và đóng môi trường MPI. Phải là hàm cuối cùng được gọi.
*   `MPI_Comm_size(MPI_COMM_WORLD, &size)`: Trả về tổng số tiến trình tham gia trong bộ truyền thông (communicator) chỉ định.
*   `MPI_Comm_rank(MPI_COMM_WORLD, &rank)`: Trả về ID (thứ hạng) của tiến trình gọi hàm ($rank \\in [0, size-1]$). Rank dùng để phân chia công việc cho từng node.

### 3. Giao tiếp Điểm-Điểm (Point-to-Point Communication)
*   **Truyền nhận Chặn (Blocking)**:
    *   `MPI_Send(buf, count, datatype, dest, tag, comm)`: Gửi dữ liệu. Hàm chỉ trả về khi vùng đệm gửi có thể tái sử dụng an toàn (thường là sau khi hệ thống đã copy dữ liệu đi hoặc đối tác đã nhận).
    *   `MPI_Recv(buf, count, datatype, source, tag, comm, status)`: Nhận dữ liệu. Chặn cho đến khi nhận đủ dữ liệu khớp với `source` và `tag`.
    *   *Deadlock*: Xảy ra khi hai hay nhiều tiến trình cùng chờ nhau nhận (hoặc gửi) thông điệp.
    *   *Cách khắc phục*:
        1.  Thay đổi trật tự Send/Recv ở các rank chẵn/lẻ.
        2.  Sử dụng `MPI_Sendrecv` kết hợp gửi nhận đồng thời.
        3.  Sử dụng giao tiếp không chặn (Non-blocking).
*   **Truyền nhận Không Chặn (Non-blocking)**:
    *   Cho phép xen kẽ tính toán và truyền thông (overlap computation and communication) để nâng cao hiệu năng.
    *   `MPI_Isend(...)` và `MPI_Irecv(...)`: Trả về ngay lập tức kèm theo một đối tượng yêu cầu (`MPI_Request`).
    *   `MPI_Wait(&request, &status)`: Chặn cho đến khi thao tác không chặn tương ứng hoàn thành.
    *   `MPI_Test(&request, &flag, &status)`: Kiểm tra nhanh xem thao tác đã hoàn thành chưa mà không chặn luồng tính toán.

### 4. Giao tiếp Tập thể (Collective Communications)
Tất cả các tiến trình trong communicator bắt buộc phải tham gia gọi các hàm này đồng thời:
*   `MPI_Barrier(comm)`: Đồng bộ hóa toàn cục. Mọi tiến trình dừng lại cho đến khi tất cả cùng chạm tới rào cản này.
*   `MPI_Bcast(buf, count, datatype, root, comm)`: Tiến trình `root` gửi bản sao dữ liệu của mình tới tất cả các tiến trình khác.
*   `MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)`: Chia đều một mảng dữ liệu từ tiến trình `root` thành các phần bằng nhau và phân phối tới từng tiến trình (mỗi tiến trình nhận 1 phần).
*   `MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm)`: Thu thập các mảnh dữ liệu từ tất cả các tiến trình và gom lại thành một mảng lớn tại tiến trình `root`.
*   `MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm)`: Thu thập dữ liệu từ tất cả các tiến trình, thực hiện phép toán thu gọn (`op` như `MPI_SUM`, `MPI_MAX`, `MPI_MIN`) và lưu kết quả tại tiến trình `root`.
*   `MPI_Allreduce(...)`: Tương đương với `MPI_Reduce` nhưng kết quả thu gọn cuối cùng được phân phối đến toàn bộ các tiến trình thay vì chỉ giữ ở `root`.
*   `MPI_Allgather(...)`: Tương đương với `MPI_Gather` nhưng mảng kết quả cuối cùng được chuyển đến toàn bộ các tiến trình.

### 5. Kiểu dữ liệu Tự định nghĩa (Derived Datatypes)
Dùng để truyền các thông điệp có cấu trúc dữ liệu không liên tục trong bộ nhớ mà không cần sao chép trung gian tốn kém:
1.  **Contiguous**: Bản sao liên tục của cùng một kiểu dữ liệu (`MPI_Type_contiguous`).
2.  **Vector**: Bản sao định kỳ với khoảng nhảy (stride) cố định giữa các khối dữ liệu (`MPI_Type_vector`). Ví dụ: trích xuất một cột của ma trận lưu trữ theo hàng.
3.  **Indexed**: Các khối dữ liệu có độ dài và khoảng cách dịch chuyển khác nhau (`MPI_Type_indexed`).
4.  **Struct**: Tổng quát nhất, cho phép định nghĩa kiểu dữ liệu chứa các phần tử có kiểu dữ liệu khác nhau và khoảng dịch chuyển bất kỳ (`MPI_Type_create_struct`).
*   **Chú ý**: Kiểu dữ liệu mới phải được đăng ký bằng `MPI_Type_commit(&newtype)` trước khi sử dụng và giải phóng bằng `MPI_Type_free(&newtype)` khi kết thúc.

### 6. Song song hóa Phân rã Miền (Domain Decomposition) & MPI-IO
*   **Phân rã miền**: Chia lưới tính toán lớn thành các lưới con nhỏ hơn gán cho các tiến trình khác nhau. Phổ biến trong giải phương trình vi phân Laplace 2D.
    *   Tạo mạng lưới Cartesian ảo bằng `MPI_Cart_create`.
    *   Xác định các láng giềng (trái, phải, trên, dưới) bằng `MPI_Cart_shift`.
    *   Các tiến trình trao đổi vùng biên (halo exchange) định kỳ thông qua `MPI_Sendrecv` hoặc không chặn.
*   **MPI-IO (Parallel I/O)**: Cho phép nhiều tiến trình truy cập đồng thời vào một tệp dùng chung để đọc/ghi song song hiệu năng cao, tránh nghẽn cổ chai khi dồn hết dữ liệu về node master ghi file tuần tự.
    *   Mở tệp dùng chung bằng `MPI_File_open`.
    *   Đọc/ghi bằng con trỏ tệp chia sẻ (`MPI_File_write_ordered`) hoặc con trỏ tệp cá nhân kết hợp seek (`MPI_File_seek` + `MPI_File_write`).

---

## PHẦN IV: THUẬT TOÁN SỐ HỌC SONG SONG & MẠNG LƯỚI (NUMERICAL ALGORITHMS & MESHES)

### 1. Phân hoạch Ma trận và Nhân Ma trận Song song
*   **Nhân ma trận tuần tự**: $C = A \\times B$. Độ phức tạp tuần tự là $O(n^3)$ cho ma trận $n \\times n$.
*   **Song song hóa trực tiếp**:
    *   Sử dụng $n^2$ bộ xử lý (mỗi bộ xử lý tính một phần tử $C[i,j]$). Thời gian chạy giảm xuống còn $O(n)$.
    *   Sử dụng $n^3$ bộ xử lý (thực hiện tất cả các phép nhân thành phần đồng thời trong $O(1)$, sau đó cộng dồn bằng cây reduction trong $O(\\log n)$ bước).
*   **Thuật toán Cannon (Cannon's Algorithm)**:
    *   Thiết kế riêng cho mạng lưới bộ xử lý dạng lưới 2D ($s \\times s$ bộ xử lý).
    *   **Quy trình căn chỉnh ban đầu (Preskewing)**:
        *   Dịch chuyển hàng $i$ của ma trận $A$ sang trái vòng tròn $i$ bước.
        *   Dịch chuyển cột $j$ của ma trận $B$ lên trên vòng tròn $j$ bước.
    *   **Quy trình nhân lặp**:
        *   Tại mỗi bước, các bộ xử lý nhân các khối dữ liệu hiện tại đang nắm giữ và cộng dồn vào kết quả khối ma trận $C$.
        *   Dịch chuyển tất cả các khối của $A$ sang trái vòng tròn 1 bước.
        *   Dịch chuyển tất cả các khối của $B$ lên trên vòng tròn 1 bước.
        *   Lặp lại $s$ lần.
    *   *Ưu điểm*: Tiết kiệm bộ nhớ vì dữ liệu dịch chuyển vòng tròn tuần tuần tự giữa các tiến trình láng giềng gần mà không cần broadcast toàn cục.

### 2. Giải Hệ Phương trình Tuyến tính
*   **Khử Gauss (Gaussian Elimination)**: biến đổi hệ phương trình về dạng tam giác.
*   **Phân chia công việc (Work Partitioning)**:
    *   *Strip Partitioning (Phân mảnh dải thẳng)*: Chia các hàng ma trận lần lượt cho các tiến trình. Nhược điểm: Khi thuật toán khử tiến dần về các hàng cuối, các tiến trình giữ các hàng đầu tiên sẽ bị nhàn rỗi (load imbalance nghiêm trọng).
    *   *Cyclic-Striped Partitioning (Phân striped vòng tròn)*: Hàng $i$ được phân cho tiến trình $i \\pmod P$. Giúp phân phối đều tải trọng tính toán trong suốt quá trình khử vì ở bất kỳ giai đoạn nào, các hàng chưa khử vẫn được chia đều cho tất cả tiến trình.
*   **Phương pháp Lặp Jacobi và Gauss-Seidel**:
    *   *Jacobi*: Tính toán các giá trị mới hoàn toàn dựa trên các giá trị của bước lặp trước. Rất dễ song song hóa vì không có sự phụ thuộc chéo trong cùng một bước lặp.
    *   *Gauss-Seidel*: Sử dụng các giá trị vừa tính mới ngay lập tức để tính các phần tử tiếp theo. Khó song song hóa trực tiếp do có sự phụ thuộc tuần tự.
    *   *Sắp xếp Đỏ-Đen (Red-Black Ordering)*: Tô màu các điểm lưới xen kẽ như bàn cờ vua. Điểm Đỏ chỉ phụ thuộc vào điểm Đen láng giềng, và ngược lại. Thuật toán có thể song song hóa trong 2 pha: tính đồng thời toàn bộ điểm Đen, sau đó tính đồng thời toàn bộ điểm Đỏ.

---

## PHẦN V: HỆ SINH THÁI DỮ LIỆU LỚN (BIG DATA HADOOP HDFS & MAPREDUCE)

### 1. HDFS (Hadoop Distributed File System) - Lưu trữ phân tán
*   **Ý tưởng chủ đạo**: Lưu trữ các tệp cực lớn (hàng Terabyte đến Petabyte) trên các cụm máy tính cấu hình phổ thông (commodity hardware) một cách tin cậy thông qua nhân bản dữ liệu (replication).
*   **Kiến trúc NameNode và DataNode (Master-Slave)**:
    *   **NameNode (Master)**: Quản lý không gian tên hệ thống tập tin (Directory Tree), lưu trữ Metadata của các tệp (vị trí các block dữ liệu trên các DataNode). Là điểm yếu chí tử của hệ thống (Single Point of Failure - SPoF), thường được backup bằng Secondary NameNode hoặc kiến trúc High Availability.
    *   **DataNode (Slave)**: Lưu trữ các khối dữ liệu thực tế (Blocks). Định kỳ gửi báo cáo trạng thái hoạt động (Heartbeat) và danh sách block hiện có về cho NameNode.
*   **Cơ chế hoạt động**:
    *   *Block Size*: Các tệp lớn được cắt thành các block có kích thước cố định rất lớn (mặc định 128MB hoặc 256MB) để giảm thiểu kích thước lưu trữ metadata trên bộ nhớ RAM của NameNode.
    *   *Replication Factor*: Hệ số nhân bản mặc định là $3$. Các bản sao block được lưu trữ trên các rack (giá đỡ) khác nhau để đảm bảo khả năng chịu lỗi tối đa (mất điện rack, hỏng node).
    *   *Luồng ghi dữ liệu (Write Pipeline)*: Khách hàng hỏi NameNode danh sách DataNode để ghi. Sau đó khách hàng đẩy block dữ liệu đến DataNode đầu tiên, DataNode này sẽ pipelining đẩy tiếp sang DataNode thứ 2 và thứ 3.

### 2. MapReduce - Mô hình xử lý phân tán đơn giản hóa
*   **Mô hình lập trình**:
    *   Chia nhỏ tính toán thành hai hàm chính do lập trình viên định nghĩa:
        1.  **Map**: Nhận một cặp $(k_1, v_1)$, sinh ra một danh sách các cặp khóa-giá trị trung gian:
            $$Map(k_1, v_1) \\to list(k_2, v_2)$$
        2.  **Reduce**: Nhận một khóa $k_2$ và danh sách tất cả các giá trị tương ứng của nó $list(v_2)$, thực hiện tổng hợp và trả về kết quả cuối cùng:
            $$Reduce(k_2, list(v_2)) \\to list(k_3, v_3)$$
*   **Quy trình thực thi MapReduce**:
    1.  **Input Split**: Chia nhỏ tệp đầu vào trên HDFS thành các phần tương ứng với các block dữ liệu.
    2.  **Map Phase**: Thực thi song song hàm Map trên các node lưu trữ block dữ liệu tương ứng (đảm bảo tính cục bộ của dữ liệu - Data Locality, không cần di chuyển dữ liệu qua mạng).
    3.  **Shuffle and Sort Phase**: Hệ thống tự động gom nhóm tất cả các cặp khóa-giá trị trung gian có cùng một khóa $k_2$, sắp xếp chúng và chuyển đến node chạy tác vụ Reduce thích hợp. Đây là pha tốn kém băng thông mạng nhất.
    4.  **Reduce Phase**: Thực thi song song hàm Reduce để tổng hợp kết quả.
    5.  **Output**: Kết quả được ghi ngược lại vào HDFS.
*   **WordCount Ví dụ điển hình**:
    *   *Map*: Nhận đầu vào là dòng văn bản. Tách từ và phát ra cặp: `(word, 1)`.
    *   *Reduce*: Nhận đầu vào là `(word, list(1, 1, 1...))`. Cộng tổng số lượng `1` để phát ra: `(word, count)`.
"""

with open(dest_file, "w", encoding="utf-8") as f:
    f.write(guide_content)

print("Master theory guide generated!")
