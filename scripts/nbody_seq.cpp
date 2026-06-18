#include <iostream>  // Thư viện nhập xuất chuẩn C++
#include <vector>    // Thư viện quản lý mảng động std::vector
#include <string>    // Thư viện xử lý chuỗi ký tự std::string
#include <cmath>     // Thư viện hàm toán học (sqrt, cos, sin,...)
#include <fstream>   // Thư viện đọc ghi file của C++
#include <sstream>   // Thư viện luồng chuỗi std::stringstream
#include <chrono>    // Thư viện đo thời gian chuẩn C++11
#include <cstdlib>   // Thư viện chuẩn C (rand, RAND_MAX,...)
#include <sys/time.h> // Thư viện đo thời gian hệ thống Unix/Linux

#define G 1.0        // Hằng số hấp dẫn tỷ lệ trong mô phỏng
#define EPSILON 0.001 // Hệ số làm mềm lực (softening factor) tránh chia cho 0 khi va chạm gần

// Cấu trúc hệ thống hạt sử dụng định dạng SoA (Structure of Arrays) để tối ưu hóa truy cập bộ nhớ cache
struct ParticleSystem {
    std::vector<double> x, y, z;    // Các mảng động lưu tọa độ x, y, z của hạt
    std::vector<double> vx, vy, vz; // Các mảng động lưu thành phần vận tốc vx, vy, vz
    std::vector<double> mass;       // Mảng động lưu khối lượng của từng hạt
    int n;                          // Tổng số lượng hạt trong hệ thống
};

// Hàm cấp phát bộ nhớ cho mảng động của hệ thống hạt
void allocate_system(ParticleSystem &sys, int n) {
    sys.n = n;               // Thiết lập tổng số lượng hạt
    sys.x.resize(n);         // Cấp phát kích thước n cho tọa độ x
    sys.y.resize(n);         // Cấp phát kích thước n cho tọa độ y
    sys.z.resize(n);         // Cấp phát kích thước n cho tọa độ z
    sys.vx.resize(n);        // Cấp phát kích thước n cho vận tốc vx
    sys.vy.resize(n);        // Cấp phát kích thước n cho vận tốc vy
    sys.vz.resize(n);        // Cấp phát kích thước n cho vận tốc vz
    sys.mass.resize(n);      // Cấp phát kích thước n cho khối lượng hạt
}

// Đọc điều kiện ban đầu của hệ hạt từ tệp văn bản dữ liệu
int read_input_file(const char *filename, ParticleSystem &sys) {
    std::ifstream file(filename); // Mở file dữ liệu đầu vào bằng luồng đọc ifstream
    if (!file.is_open()) {        // Kiểm tra xem file có mở thành công không
        std::cerr << "Lỗi: Không thể mở file đầu vào " << filename << std::endl;
        return 0;                 // Trả về 0 nếu mở file thất bại
    }

    int n = 0;                    // Biến đếm số lượng hạt (số dòng dữ liệu)
    std::string line;             // Chuỗi lưu tạm nội dung từng dòng của file
    while (std::getline(file, line)) { // Đọc từng dòng của file cho tới cuối
        if (!line.empty()) {      // Nếu dòng không trống thì tăng biến đếm số hạt
            n++;
        }
    }
    file.clear();                 // Xóa cờ trạng thái kết thúc file (EOF) của luồng đọc
    file.seekg(0, std::ios::beg); // Đưa con trỏ đọc file về lại đầu tệp tin

    allocate_system(sys, n);      // Cấp phát không gian bộ nhớ cho hệ hạt vừa đếm được

    for (int i = 0; i < n; i++) {  // Vòng lặp đọc thông số từng hạt từ file
        if (!(file >> sys.x[i] >> sys.y[i] >> sys.z[i] 
                   >> sys.vx[i] >> sys.vy[i] >> sys.vz[i] 
                   >> sys.mass[i])) { // Đọc 7 giá trị thực tương ứng cho mỗi dòng hạt
            std::cerr << "Lỗi: Định dạng dữ liệu không hợp lệ tại dòng " << i + 1 << std::endl;
            file.close();         // Đóng file
            return 0;             // Trả về lỗi định dạng thất bại
        }
    }

    file.close();                 // Đóng tệp dữ liệu sau khi đọc thành công
    return n;                     // Trả về tổng số hạt đã nạp vào hệ thống
}

// Tạo điều kiện ban đầu ngẫu nhiên nếu không sử dụng file đầu vào (Hệ thống đĩa Keplerian dự phòng)
void init_random_system(ParticleSystem &sys, int n) {
    allocate_system(sys, n);      // Khởi tạo và cấp phát bộ nhớ cho n hạt
    int half_n = n / 2;           // Phân nửa số hạt thuộc về thiên hà 1, nửa còn lại thuộc thiên hà 2

    for (int i = 0; i < n; i++) {  // Vòng lặp khởi tạo tọa độ và vận tốc ban đầu cho từng hạt
        double theta = ((double)rand() / RAND_MAX) * 2.0 * M_PI; // Sinh góc theta ngẫu nhiên từ 0 đến 2pi
        double r = 5.0 + ((double)rand() / RAND_MAX) * 25.0;     // Sinh bán kính r ngẫu nhiên từ 5 đến 30 đơn vị
        
        if (i < half_n) {         // Cài đặt các thông số cho hạt thuộc Thiên hà 1 (nằm bên trái)
            sys.x[i] = -50.0 + r * cos(theta); // Tọa độ x lệch về tâm -50
            sys.y[i] = r * sin(theta);         // Tọa độ y xoay tròn quanh tâm
            sys.z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0; // Tọa độ z (độ dày đĩa ngẫu nhiên)
            
            double v_orbit = sqrt(G * 22000.0 / r); // Tính vận tốc quỹ đạo Keplerian v = sqrt(G*M/r)
            sys.vx[i] = 3.2 - v_orbit * sin(theta); // Thành phần vận tốc vx kèm theo vận tốc tịnh tiến thiên hà
            sys.vy[i] = 0.6 + v_orbit * cos(theta); // Thành phần vận tốc vy kèm theo vận tốc tịnh tiến thiên hà
            sys.vz[i] = 0.0;                        // Thành phần vận tốc vz ban đầu bằng 0
            sys.mass[i] = (i == 0) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9; // Hạt 0 là lỗ đen siêu nặng
        } else {                  // Cài đặt các thông số cho hạt thuộc Thiên hà 2 (nằm bên phải)
            sys.x[i] = 50.0 + r * cos(theta);  // Tọa độ x lệch về tâm +50
            sys.y[i] = r * sin(theta);         // Tọa độ y xoay tròn quanh tâm
            sys.z[i] = ((double)rand() / RAND_MAX) * 2.0 - 1.0; // Tọa độ z (độ dày đĩa ngẫu nhiên)
            
            double v_orbit = sqrt(G * 22000.0 / r); // Tính vận tốc quỹ đạo Keplerian v = sqrt(G*M/r)
            sys.vx[i] = -3.2 + v_orbit * sin(theta); // Thành phần vận tốc vx kèm theo vận tốc tịnh tiến ngược chiều
            sys.vy[i] = -0.6 - v_orbit * cos(theta); // Thành phần vận tốc vy kèm theo vận tốc tịnh tiến ngược chiều
            sys.vz[i] = 0.0;                         // Thành phần vận tốc vz ban đầu bằng 0
            sys.mass[i] = (i == half_n) ? 22000.0 : 0.1 + ((double)rand() / RAND_MAX) * 0.9; // Hạt half_n là lỗ đen 2
        }
    }
}

// Tính toán lực hấp dẫn tương tác trực tiếp O(N^2) và cập nhật hệ thức tích phân Leapfrog
void update_particles(ParticleSystem &sys, double dt) {
    int n = sys.n; // Số lượng hạt trong hệ
    
    std::vector<double> ax(n, 0.0); // Mảng lưu trữ thành phần gia tốc ax
    std::vector<double> ay(n, 0.0); // Mảng lưu trữ thành phần gia tốc ay
    std::vector<double> az(n, 0.0); // Mảng lưu trữ thành phần gia tốc az

    // Tính toán gia tốc hấp dẫn dựa trên luật tương tác trực tiếp
    #pragma omp parallel for
    for (int i = 0; i < n; i++) { // Duyệt qua từng hạt i chịu lực
        double acc_x = 0.0;
        double acc_y = 0.0;
        double acc_z = 0.0;
        
        double xi = sys.x[i];
        double yi = sys.y[i];
        double zi = sys.z[i];

        for (int j = 0; j < n; j++) { // Duyệt qua tất cả các hạt j gây lực lên i
            if (i == j) continue; // Bỏ qua tương tác của hạt với chính nó
            
            double dx = sys.x[j] - xi; // Khoảng cách thành phần delta_x
            double dy = sys.y[j] - yi; // Khoảng cách thành phần delta_y
            double dz = sys.z[j] - zi; // Khoảng cách thành phần delta_z
            
            double distSqr = dx*dx + dy*dy + dz*dz + EPSILON*EPSILON; // Bình phương khoảng cách cộng hệ số làm mềm
            double invDist = 1.0 / sqrt(distSqr); // Nghịch đảo khoảng cách 1/r
            double invDistCube = invDist * invDist * invDist; // Nghịch đảo khoảng cách lập phương 1/r^3
            
            double f = G * sys.mass[j] * invDistCube; // Tính hệ số lực hấp dẫn tương ứng
            
            acc_x += dx * f; // Tích lũy gia tốc thành phần x
            acc_y += dy * f; // Tích lũy gia tốc thành phần y
            acc_z += dz * f; // Tích lũy gia tốc thành phần z
        }
        ax[i] = acc_x; // Lưu trữ gia tốc x cuối cùng của hạt i
        ay[i] = acc_y; // Lưu trữ gia tốc y cuối cùng của hạt i
        az[i] = acc_z; // Lưu trữ gia tốc z cuối cùng của hạt i
    }

    // Cập nhật vận tốc và tọa độ hạt theo phương pháp tích phân Leapfrog
    for (int i = 0; i < n; i++) { // Duyệt qua từng hạt để cập nhật động học
        sys.vx[i] += ax[i] * dt; // Cập nhật vận tốc vx = vx + ax * dt
        sys.vy[i] += ay[i] * dt; // Cập nhật vận tốc vy = vy + ay * dt
        sys.vz[i] += az[i] * dt; // Cập nhật vận tốc vz = vz + az * dt
        
        sys.x[i] += sys.vx[i] * dt; // Cập nhật tọa độ x = x + vx * dt
        sys.y[i] += sys.vy[i] * dt; // Cập nhật tọa độ y = y + vy * dt
        sys.z[i] += sys.vz[i] * dt; // Cập nhật tọa độ z = z + vz * dt
    }
}

// Đo thời gian hệ thống tính bằng giây
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL); // Lấy thời gian từ đồng hồ hệ thống
    return tv.tv_sec + tv.tv_usec * 1e-6; // Chuyển sang giá trị giây kiểu double
}

int main(int argc, char **argv) {
    if (argc < 4) { // Kiểm tra đối số dòng lệnh đầu vào
        std::cout << "Sử dụng: " << argv[0] << " <input_file_or_N> <steps> <dt>" << std::endl;
        std::cout << "Ví dụ 1 (đọc file): " << argv[0] << " ../Data/galaxy_collision_10k.txt 100 0.01" << std::endl;
        std::cout << "Ví dụ 2 (tự động tạo): " << argv[0] << " 10000 100 0.01" << std::endl;
        return 1;
    }

    ParticleSystem sys;  // Khai báo cấu trúc hệ thống hạt
    int n = 0;           // Số lượng hạt ban đầu
    int steps = atoi(argv[2]);  // Số bước nhảy thời gian mô phỏng
    double dt = atof(argv[3]);  // Độ dài bước thời gian tích phân

    char *endptr;
    int parsed_n = strtol(argv[1], &endptr, 10); // Thử phân tách tham số đầu tiên thành số nguyên

    double start_init = get_time(); // Đo mốc bắt đầu khởi tạo
    if (*endptr == '\0') { // Nếu tham số thứ nhất hoàn toàn là số nguyên thì sinh ngẫu nhiên
        n = parsed_n;
        std::cout << "Khởi tạo hệ thống ngẫu nhiên với " << n << " hạt..." << std::endl;
        init_random_system(sys, n); // Thực thi sinh dữ liệu đĩa Keplerian ngẫu nhiên
    } else { // Nếu là một đường dẫn tệp tin thì tiến hành đọc dữ liệu từ file
        const char *filename = argv[1];
        std::cout << "Đang đọc điều kiện ban đầu từ file: " << filename << "..." << std::endl;
        n = read_input_file(filename, sys); // Đọc tọa độ từ tệp tin
        if (n == 0) { // Thoát chương trình nếu đọc file lỗi
            return 1;
        }
        std::cout << "Nạp thành công " << n << " hạt." << std::endl;
    }
    double end_init = get_time(); // Đo mốc kết thúc khởi tạo
    std::cout << "Thời gian khởi tạo hệ thống: " << (end_init - start_init) << " giây." << std::endl;

    std::cout << "Bắt đầu chạy mô phỏng: " << steps << " bước, dt = " << dt << "..." << std::endl;
    double start_sim = get_time(); // Đo mốc thời gian bắt đầu chạy vòng lặp mô phỏng
    for (int step = 0; step < steps; step++) { // Thực thi các bước mô phỏng
        update_particles(sys, dt); // Gọi hàm cập nhật lực và động học
        if ((step + 1) % 10 == 0 || step == 0) { // Định kỳ hiển thị tiến độ mô phỏng
            std::cout << "Hoàn thành bước mô phỏng " << step + 1 << "/" << steps << "..." << std::endl;
        }
    }
    double end_sim = get_time(); // Đo mốc kết thúc mô phỏng
    
    double elapsed_sim = end_sim - start_sim; // Tính tổng thời gian mô phỏng thực tế
    std::cout << "\nMô phỏng kết thúc." << std::endl;
    std::cout << "Tổng thời gian chạy mô phỏng: " << elapsed_sim << " giây." << std::endl;
    // Tính toán hiệu năng quy đổi ra Mflops/sec
    std::cout << "Hiệu năng: " << ((double)steps * n * n * 20.0 / (elapsed_sim * 1e6)) << " Mflops/sec quy đổi" << std::endl;

    // Hiển thị tọa độ vị trí tâm của 2 thiên hà ở cuối mô phỏng để kiểm tra tính đúng đắn
    std::cout << "\nTọa độ cuối cùng của Tâm Thiên hà 1 (Hạt số 0): (" 
              << sys.x[0] << ", " << sys.y[0] << ", " << sys.z[0] << ")" << std::endl;
    if (n > n/2) {
        std::cout << "Tọa độ cuối cùng của Tâm Thiên hà 2 (Hạt số " << n/2 << "): (" 
                  << sys.x[n/2] << ", " << sys.y[n/2] << ", " << sys.z[n/2] << ")" << std::endl;
    }

    return 0; // Kết thúc chương trình
}
