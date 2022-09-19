#include "adma_ros2_driver/adma_driver.hpp"
#include "adma_ros2_driver/adma_parse.hpp"
#include <rclcpp_components/register_node_macro.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

namespace genesys
{
        ADMADriver::ADMADriver(const rclcpp::NodeOptions &options) : Node("adma_driver", options),
        _rcvSockfd(-1),
        _rcvAddrInfo(NULL),
        _admaAddr(),
        _admaAddrLen(4),
        _admaPort(0)
        {
                std::string param_address = this->declare_parameter("destination_ip", "0.0.0.0");
                _admaPort = this->declare_parameter("destination_port", 1040);
                initializeUDP(param_address);

                _performance_check = this->declare_parameter("use_performance_check", false);
                _gnss_frame = this->declare_parameter("gnss_frame", "gnss_link");
                _imu_frame = this->declare_parameter("imu_frame", "imu_link");

                _pub_adma_data = this->create_publisher<adma_msgs::msg::AdmaData>("adma/data", 1);
                _pub_navsat_fix = this->create_publisher<sensor_msgs::msg::NavSatFix>("adma/fix", 1);
                _pub_imu = this->create_publisher<sensor_msgs::msg::Imu>("adma/imu", 1);
                _pub_heading = this->create_publisher<std_msgs::msg::Float64>("adma/heading", 1);
                _pub_velocity = this->create_publisher<std_msgs::msg::Float64>("adma/velocity", 1);

                updateLoop();
        }

        ADMADriver::~ADMADriver(){
                freeaddrinfo(_rcvAddrInfo);
                ::shutdown(_rcvSockfd, SHUT_RDWR);
                _rcvSockfd = -1;
        }

        void ADMADriver::initializeUDP(std::string admaAdress)
        {
                struct addrinfo hints;
                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_DGRAM;
                hints.ai_protocol = IPPROTO_UDP;
                std::string rcvPortStr = std::to_string(_admaPort);


                _admaAddrLen= sizeof(_admaAddr);
                memset((char *) &_admaAddr, 0, _admaAddrLen);
                _admaAddr.sin_family = AF_INET;
                _admaAddr.sin_port = htons(_admaPort);
                inet_aton(admaAdress.c_str(), &(_admaAddr.sin_addr));

                int r = getaddrinfo(admaAdress.c_str(), rcvPortStr.c_str(), &hints, &_rcvAddrInfo);
                if (r != 0 || _rcvAddrInfo == NULL) {
                        RCLCPP_FATAL(get_logger(), "Invalid port for UDP socket: \"%s:%s\"", admaAdress.c_str(), rcvPortStr.c_str());
                        throw rclcpp::exceptions::InvalidParameterValueException("Invalid port for UDP socket: \"" + admaAdress + ":" + rcvPortStr + "\"");
                }
                _rcvSockfd = socket(_rcvAddrInfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
                if (_rcvSockfd == -1) {
                        freeaddrinfo(_rcvAddrInfo);
                        RCLCPP_FATAL(get_logger(), "Could not create UDP socket for: \"%s:%s", admaAdress.c_str(), rcvPortStr.c_str());
                        throw rclcpp::exceptions::InvalidParameterValueException("Could not create UDP socket for: \"" + admaAdress + ":" + rcvPortStr + "\"");
                }
                r = bind(_rcvSockfd, _rcvAddrInfo->ai_addr, _rcvAddrInfo->ai_addrlen);
                if (r != 0) {
                        freeaddrinfo(_rcvAddrInfo);
                        ::shutdown(_rcvSockfd, SHUT_RDWR);
                        RCLCPP_FATAL(get_logger(), "Could not bind UDP socket with: \"%s:%s", admaAdress.c_str(), rcvPortStr.c_str());
                        throw rclcpp::exceptions::InvalidParameterValueException("Could not bind UDP socket with: \"" + admaAdress + ":" + rcvPortStr + "\"");
                }
        }

        void ADMADriver::updateLoop()
        {
                fd_set s;
                struct timeval timeout;
                // AdmaData admaData;
                std::array<char, 856> recv_buf;
                struct sockaddr srcAddr;
                socklen_t srcAddrLen;

                while (rclcpp::ok())
                {
                        // check if new data is available
                        FD_ZERO(&s);
                        FD_SET(_rcvSockfd, &s);
                        timeout.tv_sec = 1;
                        timeout.tv_usec = 0;
                        int ret = select(_rcvSockfd + 1, &s, NULL, NULL, &timeout);
                        if (ret == 0) {
                                // reached timeout
                                RCLCPP_INFO(get_logger(), "Waiting for ADMA data...");
                                continue;
                        } else if (ret == -1) {
                                // error
                                RCLCPP_WARN(get_logger(), "Select-error: %s", strerror(errno));
                                continue;
                        }

                        ret = ::recvfrom(_rcvSockfd, (void *) (&recv_buf), sizeof(recv_buf), 0, &srcAddr, &srcAddrLen);

                        if (ret < 0) {
                                RCLCPP_WARN(get_logger(), "Receive-error: %s", strerror(errno));
                                continue;
                        } else if (ret != sizeof(recv_buf)) {
                                RCLCPP_WARN(get_logger(), "Invalid ADMA message size: %d instead of %ld", ret, sizeof(recv_buf));
                                continue;
                        }


                        builtin_interfaces::msg::Time curTimestamp = this->get_clock()->now();

                        /* Prepare for parsing */
                        std::string local_data(recv_buf.begin(), recv_buf.end());
                        /* Load the messages on the publishers */
                        // adma_msgs::msg::AdmaData message;
                        // sensor_msgs::msg::NavSatFix message_fix;
                        // message_fix.header.stamp = this->now();
                        // message_fix.header.frame_id = "adma";
                        // std_msgs::msg::Float64 message_heading;
                        // std_msgs::msg::Float64 message_velocity;
                        // message.timemsec = this->get_clock()->now().seconds() * 1000;
                        // message.timensec = this->get_clock()->now().nanoseconds();
                        // getparseddata(local_data, message, message_fix, message_heading, message_velocity);
                        
                        // /* publish the ADMA message */
                        // _pub_adma_data->publish(message);
                        // _pub_navsat_fix->publish(message_fix);
                        // _pub_heading->publish(message_heading);
                        // _pub_velocity->publish(message_velocity);
                        // double grab_time = this->get_clock()->now().seconds();

                        // if (_performance_check)
                        // {
                        //         char ins_time_msec[] = {local_data[584], local_data[585], local_data[586], local_data[587]};
                        //         memcpy(&message.instimemsec, &ins_time_msec, sizeof(message.instimemsec));
                        //         float weektime = message.instimeweek;
                        //         RCLCPP_INFO(get_logger(), "%f ", ((grab_time * 1000) - (message.instimemsec + 1592697600000)));
                        // }

                        adma_msgs::msg::AdmaData message_admadata;
                        message_admadata.timemsec = this->get_clock()->now().seconds() * 1000;
                        message_admadata.timensec = this->get_clock()->now().nanoseconds();
                        sensor_msgs::msg::NavSatFix message_navsatfix;
                        message_navsatfix.header.stamp = this->get_clock()->now();
                        message_navsatfix.header.frame_id = _gnss_frame;
                        sensor_msgs::msg::Imu message_imu;
                        message_imu.header.stamp = this->get_clock()->now();
                        message_imu.header.frame_id = _imu_frame;

                        std_msgs::msg::Float64 message_heading;
                        std_msgs::msg::Float64 message_velocity;
                        getparseddata(local_data, message_admadata, message_navsatfix, message_imu, message_heading, message_velocity);

                        /* publish the ADMA message */
                        _pub_adma_data->publish(message_admadata);
                        _pub_navsat_fix->publish(message_navsatfix);
                        _pub_imu->publish(message_imu);
                        _pub_heading->publish(message_heading);
                        _pub_velocity->publish(message_velocity);

                        if (_performance_check)
                        {
                        double grab_time = this->get_clock()->now().seconds();
                        char ins_time_msec[] = { local_data[584], local_data[585], local_data[586], local_data[587] };
                        memcpy(&message_admadata.instimemsec, &ins_time_msec, sizeof(message_admadata.instimemsec));
                        float weektime = message_admadata.instimeweek;
                        RCLCPP_INFO(this->get_logger(), "%f ", ((grab_time * 1000) - (message_admadata.instimemsec + 1592697600000)));
                        }
                }
        }
} // namespace genesys

RCLCPP_COMPONENTS_REGISTER_NODE(genesys::ADMADriver)