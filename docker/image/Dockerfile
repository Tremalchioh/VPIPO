FROM ubuntu:20.04

COPY install_deps.sh /
RUN sh install_deps.sh

ENV TZ=Europe/Moscow

ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

ENV CXX="/usr/bin/clang++-12"
ENV CC="/usr/bin/clang-12"

# User for CLion
RUN groupadd -g 712342 clion_user_group
RUN useradd -u 712342 -g 712342 -m clion_user && yes password | passwd clion_user

# SSH
EXPOSE 22
RUN echo "StrictHostKeyChecking=no" >> /etc/ssh/ssh_config
RUN mkdir /var/run/sshd
CMD ["/usr/sbin/sshd", "-D"]
