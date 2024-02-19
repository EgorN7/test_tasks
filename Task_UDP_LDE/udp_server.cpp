#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <fstream>
#include <optional>
#include <thread>
#include <type_traits>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

//Класс для парсинга базы данных,
// в дальнейшем можно даписать методы для получения данных из файлов других форматов  
class Base {
public:
    void ParseTxt(const std::string& file_path) {
        std::ifstream in;
        in.open(file_path);

        if (in.is_open()) {

            std::string line;
            while (std::getline(in, line)) {
                int delimiter_position = line.find(' ');
                data_[line.substr(0, delimiter_position)] = line.substr(delimiter_position + 1);
            }

            in.close();
        }
    }

    //При использовании стандарта С++20 метод unordered_map.count, нужно заменить на unordered_map.contains
    std::optional<std::string> FindResourse(const std::string& searched_resource) {
        if (data_.count(searched_resource)) {
            return data_[searched_resource];
        }
        return std::nullopt;
    }

private:
//Так как у нас из разных потоков происходит только чтение,
// то мы можем использовать unordered_map, не опасаясь состояния гонки у потоков
    std::unordered_map<std::string, std::string> data_;
};

template <typename T>
std::string MakeMessege(const T& messege_body) {
    if constexpr (std::is_same_v<T, std::string>) {
        return "-BEGIN-\n"s + messege_body + "\n-END-"s;
    } else {
        return "-ERROR-\n"s + messege_body.message() + "\n-END-"s;
    }
}


int main() {

    static const int port = 3333;
    static const size_t max_buffer_size = 64;
    static const std::string file_path = "data_base.txt"s;

    Base base;
    base.ParseTxt(file_path);

    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        for (;;) {

            udp::endpoint remote_endpoint;
            std::array<char, max_buffer_size> recv_buf;

            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            std::thread t(
                [&base](std::string need_resours, udp::endpoint remote_endpoint) {
                    boost::asio::io_context io_context_tmp;
                    boost::system::error_code ec;
                    udp::socket socket_send(io_context_tmp, udp::v4());
                    std::optional<std::string> resourse_data = base.FindResourse(need_resours);
                    if (resourse_data) {
                        socket_send.send_to(
                            boost::asio::buffer(MakeMessege(*resourse_data)), remote_endpoint, 0, ec);
                    }
                    else {
                        ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
                    }

                    if (ec) {
                        socket_send.send_to(
                            boost::asio::buffer(MakeMessege(ec)), remote_endpoint, 0);
                    }

                },
                std::move(std::string(recv_buf.data(), size)), std::move(remote_endpoint));

            t.detach();

        }

    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
//В зависимости от требований к проекту нужно выбирать подходящую стратегию написания кода
//К примеру: для увеличения производительности можно уменьшить количество функций, для уменьшение времени, 
//которое тратиться на выделение ресурсов для работы функции, но это уменьшит читаемость кода
//или можно заняться управлением памятью, для оптимизации работы процессора с кешированными данными.