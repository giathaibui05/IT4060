/*
 * sv_client.c
 * Bài 3: Nhập thông tin sinh viên (MSSV, họ tên, ngày sinh, điểm TB),
 * đóng gói và gửi sang sv_server.
 *
 * Cách chạy: ./sv_client <địa chỉ IP> <cổng>
 * Ví dụ:     ./sv_client 127.0.0.1 9090
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MSSV_LEN    20
#define HOTEN_LEN  100
#define DATE_LEN    12   /* YYYY-MM-DD */
#define BUF_LEN    256

/* Cấu trúc thông tin sinh viên (dùng để đóng gói) */
typedef struct {
    char  mssv[MSSV_LEN];
    char  ho_ten[HOTEN_LEN];
    char  ngay_sinh[DATE_LEN];   /* định dạng YYYY-MM-DD */
    float diem_tb;
} SinhVien;

/*
 * Serialize SinhVien thành chuỗi văn bản một dòng:
 * "MSSV HoTen NgaySinh DiemTB\n"
 * Ví dụ: "20201234 Nguyen Van A 2002-04-10 3.99\n"
 */
static int sv_to_string(const SinhVien *sv, char *buf, int buf_size) {
    return snprintf(buf, buf_size, "%s %s %s %.2f\n",
                    sv->mssv, sv->ho_ten, sv->ngay_sinh, sv->diem_tb);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Cach dung: %s <dia chi IP> <cong>\n", argv[0]);
        fprintf(stderr, "Vi du:     %s 127.0.0.1 9090\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    /* ---- Tạo socket và kết nối ---- */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket()"); exit(EXIT_FAILURE); }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", ip);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[sv_client] Da ket noi den sv_server %s:%d\n\n", ip, port);

    /* ---- Nhập thông tin sinh viên ---- */
    SinhVien sv;
    memset(&sv, 0, sizeof(sv));

    printf("--- Nhap thong tin sinh vien ---\n");

    printf("MSSV             : ");
    if (scanf("%19s", sv.mssv) != 1) {
        fprintf(stderr, "Loi nhap MSSV.\n");
        close(sockfd); exit(EXIT_FAILURE);
    }
    getchar(); /* bỏ ký tự '\n' còn lại trong buffer */

    printf("Ho va ten        : ");
    if (fgets(sv.ho_ten, HOTEN_LEN, stdin) == NULL) {
        fprintf(stderr, "Loi nhap ho ten.\n");
        close(sockfd); exit(EXIT_FAILURE);
    }
    sv.ho_ten[strcspn(sv.ho_ten, "\n")] = '\0'; /* xóa ký tự xuống dòng */

    printf("Ngay sinh (YYYY-MM-DD): ");
    if (scanf("%11s", sv.ngay_sinh) != 1) {
        fprintf(stderr, "Loi nhap ngay sinh.\n");
        close(sockfd); exit(EXIT_FAILURE);
    }

    printf("Diem trung binh  : ");
    if (scanf("%f", &sv.diem_tb) != 1) {
        fprintf(stderr, "Loi nhap diem.\n");
        close(sockfd); exit(EXIT_FAILURE);
    }

    /* ---- Đóng gói thành chuỗi và gửi ---- */
    char payload[BUF_LEN];
    int  len = sv_to_string(&sv, payload, BUF_LEN);
    if (len <= 0) {
        fprintf(stderr, "Loi dong goi du lieu.\n");
        close(sockfd); exit(EXIT_FAILURE);
    }

    if (send(sockfd, payload, len, 0) < 0) {
        perror("send()");
        close(sockfd); exit(EXIT_FAILURE);
    }

    printf("\n[sv_client] Da gui du lieu:\n  %s", payload);

    close(sockfd);
    printf("[sv_client] Dong ket noi.\n");
    return 0;
}
