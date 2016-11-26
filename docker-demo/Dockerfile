FROM ubuntu:16.04

RUN apt-get update 
RUN apt-get install -y curl unzip sudo markdown
RUN apt-get install -y git apache2 apache2-dev


EXPOSE 81


RUN mkdir -p /var/www/html
RUN ln -s /var/www/html /html
VOLUME /html

ADD vhost.txt /
RUN sudo cat vhost.txt >> /etc/apache2/sites-available/000-default.conf

ADD ports.txt /
RUN sudo cat ports.txt >> /etc/apache2/ports.conf

ADD *.md /html/

ADD *.sh /
RUN sudo chmod +x *.sh

RUN mkdir -p ~/repo/
RUN /setup.sh

ENTRYPOINT /startup.sh


