#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

int main(int argc, const char** argv) {
    static const int port = 3333;
    static const size_t max_buffer_size = 1024;

    if (argc != 2) {
        std::cout << "Usage: "sv << argv[0] << " <intrestid word>"sv << std::endl;
        return 1;
    }

    try {
        net::io_context io_context;
        boost::system::error_code ec;
        std::array<char, max_buffer_size> recv_buf;

        udp::socket socket(io_context, udp::v4());
        auto endpoint = udp::endpoint(udp::v4(), port);
        socket.send_to(boost::asio::buffer(std::string_view(argv[1])), endpoint);
        
        udp::endpoint sender_endpoint;
        size_t size = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);

        std::cout << std::string_view(recv_buf.data(), size) << std::endl;
        
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}