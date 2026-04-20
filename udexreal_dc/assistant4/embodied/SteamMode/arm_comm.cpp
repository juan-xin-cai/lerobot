#include "arm_comm.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <errno.h>

static int tcp_sock = -1;
static struct sockaddr_in tcp_addr;

void init_arm_comm() {
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(5006);
    inet_pton(AF_INET, "127.0.0.1", &tcp_addr.sin_addr);
    // 不立即连接，在主循环中按需连接
}

static bool connect_tcp() {
    if (tcp_sock != -1) close(tcp_sock);
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("arm_comm socket");
        return false;
    }
    int flags = fcntl(tcp_sock, F_GETFL, 0);
    fcntl(tcp_sock, F_SETFL, flags | O_NONBLOCK);
    int ret = connect(tcp_sock, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr));
    if (ret < 0 && errno != EINPROGRESS) {
        perror("arm_comm connect");
        close(tcp_sock);
        tcp_sock = -1;
        return false;
    }
    printf("[ARM COMM] Connecting...\n");
    return true;
}

static const char* json_bool(bool value) {
    return value ? "true" : "false";
}

bool send_pose_to_arm(const ArmControlFrame& control_frame) {
    // 尝试连接（如果未连接或已断开）
    if (tcp_sock == -1) {
        connect_tcp();
        if (tcp_sock == -1) return false;
    }

    // 单行 JSON 协议，使用 '\n' 作为帧边界。
    char json_buf[2048];
    const char* frame_name = control_frame.frame != nullptr ? control_frame.frame : "airbot_base";
    int json_len = snprintf(
        json_buf,
        sizeof(json_buf),
        "{\"version\":1,\"timestamp_ms\":%lld,\"frame\":\"%s\",\"tracking_ok\":%s,"
        "\"following\":%s,\"pose\":{\"pos\":[%.6f,%.6f,%.6f],\"quat_wxyz\":[%.6f,%.6f,%.6f,%.6f]},"
        "\"analog\":{\"trigger\":%.6f,\"grip\":%.6f,\"joystick_x\":%.6f,\"joystick_y\":%.6f},"
        "\"buttons\":{\"trigger\":%s,\"grip\":%s,\"a\":%s,\"b\":%s,\"menu\":%s,\"joystick_press\":%s},"
        "\"events\":{\"reset_robot\":%s,\"recenter_vr\":%s}}\n",
        static_cast<long long>(control_frame.timestamp_ms),
        frame_name,
        json_bool(control_frame.tracking_ok),
        json_bool(control_frame.following),
        control_frame.pose.pos[0], control_frame.pose.pos[1], control_frame.pose.pos[2],
        control_frame.pose.quat_wxyz[0], control_frame.pose.quat_wxyz[1],
        control_frame.pose.quat_wxyz[2], control_frame.pose.quat_wxyz[3],
        control_frame.analog.trigger, control_frame.analog.grip,
        control_frame.analog.joystick_x, control_frame.analog.joystick_y,
        json_bool(control_frame.buttons.trigger),
        json_bool(control_frame.buttons.grip),
        json_bool(control_frame.buttons.a),
        json_bool(control_frame.buttons.b),
        json_bool(control_frame.buttons.menu),
        json_bool(control_frame.buttons.joystick_press),
        json_bool(control_frame.events.reset_robot),
        json_bool(control_frame.events.recenter_vr));
    if (json_len < 0 || static_cast<size_t>(json_len) >= sizeof(json_buf)) {
        printf("[ARM COMM] JSON frame too large.\n");
        return false;
    }

    ssize_t sent = send(tcp_sock, json_buf, static_cast<size_t>(json_len), 0);
    if (sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("arm_comm send");
            close(tcp_sock);
            tcp_sock = -1;
        }
        return false;
    }
    if (sent != json_len) {
        printf("[ARM COMM] Partial frame send: %zd/%d\n", sent, json_len);
        close(tcp_sock);
        tcp_sock = -1;
        return false;
    }

    // 尝试接收响应（非阻塞）
    char resp[256];
    ssize_t recv_len = recv(tcp_sock, resp, sizeof(resp)-1, MSG_DONTWAIT);
    if (recv_len > 0) {
        resp[recv_len] = '\0';
        if (strstr(resp, "\"status\":\"ok\"") != nullptr) {
            return true;
        } else {
            printf("[ARM COMM] Server error: %s\n", resp);
            return false;
        }
    } else if (recv_len == 0) {
        close(tcp_sock);
        tcp_sock = -1;
        return false;
    }
    // 未收到响应也算发送成功（服务端可能不返回）
    return true;
}

void close_arm_comm() {
    if (tcp_sock != -1) close(tcp_sock);
    tcp_sock = -1;
}
