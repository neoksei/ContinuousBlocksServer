#include "FileSystem.h"
#include <iostream>
#include <fstream>
#include <zmq.hpp>


int main() {
    FileSystem my_fs("../ContinuousDrive", 512);
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);

    socket.bind("tcp://*:60017");

    while (true) {
        zmq::message_t request;
        socket.recv(request);
        std::string command(static_cast<char *>(request.data()), request.size());
        std::stringstream ss;
        ss << command;
        std::string operation;
        ss >> operation;

        if (operation == "read") {
            std::string file;
            ss >> file; // получаем имя файла

            if (my_fs.file_exists(file.c_str())) {
                zmq::message_t reply("Success!", 8); //успешная ветка
                socket.send(reply, zmq::send_flags::none);
                socket.recv(request);

                std::string readiness(static_cast<char *>(request.data()), request.size());
                if (readiness == "Ready!") {
                    reply.rebuild(static_cast<void *>(my_fs.read(file.c_str())), my_fs.get_file_size(file.c_str()));
                    socket.send(reply, zmq::send_flags::none);
                } else {
                    reply.rebuild("Fail!", 5);
                    socket.send(reply, zmq::send_flags::none);
                }

            } else {
                zmq::message_t reply("Fail", 5); //неудачная ветка
                socket.send(reply, zmq::send_flags::none);
            }

        } else if (operation == "write") {
            std::string file;
            ss >> file; // получаем имя файла
            zmq::message_t reply("Ready!", 6);
            socket.send(reply, zmq::send_flags::none);
            socket.recv(request);
            if (my_fs.write(file.c_str(), static_cast<char *>(request.data()), request.size()) != FS_FAIL) {
                reply.rebuild("Success!", 8); //успешная ветка
                socket.send(reply, zmq::send_flags::none);
            } else {
                reply.rebuild("Fail!", 5); //неудачная ветка
                socket.send(reply, zmq::send_flags::none);
            }

        } else if (operation == "dump") {
            my_fs.dump();
            zmq::message_t reply("Success!", 8);
            socket.send(reply, zmq::send_flags::none);

        } else {
            zmq::message_t reply("Fail", 5); //неудачная ветка
            socket.send(reply, zmq::send_flags::none);
        }
    }

    return 0;
}
