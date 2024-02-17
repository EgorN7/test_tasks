#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <fstream>
#include <optional>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

//Класс для парсинга базы данных,
// в дальнейшем можно даписать методы для получения данных из файлов других форматов  
class Base{
public:
    void ParseTxt(const std::string& file_path){
        std::ifstream in;
        in.open(file_path); 

        if (in.is_open()){

            std::string line;
            while (std::getline(in, line)){
                int delimiter_position = line.find(' ');
                data_[line.substr(0,delimiter_position)] = line.substr(delimiter_position + 1);
            }

            in.close();
        }
    }

    std::optional<std::string> FindResourse(const std::string& searched_resource){
        if (data_.count(searched_resource)){
            return data_[searched_resource];
        }
        return std::nullopt;
    }

private:
    std::map<std::string,std::string> data_;
};

int main() {

    static const int port = 3333;
    static const size_t max_buffer_size = 64;
    static const file_path = "data_base.txt"s;

    Base base;
    base.ParseTxt(file_path);

    try {
        boost::asio::io_context io_context;
        
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));


        // лямбда функции для обрамления содержимого ресурсов и ошибок для отправки ответов
        // При расширее проекта можно заменить лямбда функции на класс с различными методами рбрамления
        auto make_messege_response = [](const std::string& need_a){
            return "-BEGIN-\n"s+need_a+"\n-END-"s;
        };

        auto make_messege_error = [](const std::string& error_str){
            return "-ERROR-\n"s+error_str+"\n-END-"s;
        };

        for (;;) {
            
            udp::endpoint remote_endpoint;
            std::array<char, max_buffer_size> recv_buf;
            boost::system::error_code ec;

            auto sent_error_messege = [&socket, &remote_endpoint, &make_messege_error]
            (const boost::system::error_code& current_error_code){
                socket.send_to(
                        boost::asio::buffer(make_messege_error(current_error_code.message())), remote_endpoint, 0);;
            };

            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            std::optional<std::string> resourse_data = base.FindResourse(std::string(recv_buf.data(), size));
            if (resourse_data){            
                socket.send_to(
                    boost::asio::buffer(make_messege_response(*resourse_data)), remote_endpoint, 0, ec);
            } else {
                ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
            }

            if(ec){
                sent_error_messege(ec);
            }
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
//В зависимости от требований к проекту нужно выбирать подходящую стратегию написания кода
//К примеру: для увеличения производительности можно уменьшить количество функций, для уменьшение времени, 
//которое тратиться на выделение ресурсов для работы функции, но это уменьшит читаемость кода
//или можно заняться управлением памятью, для оптимизации работы процессора с кешированными данными.