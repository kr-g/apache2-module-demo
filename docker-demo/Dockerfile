FROM ubuntu:16.04

RUN apt-get update 
RUN apt-get install -y curl unzip sudo markdown
RUN apt-get install -y git apache2 apache2-dev
RUN apt-get install -y autoconf2.13


EXPOSE 81
EXPOSE 82


RUN mkdir -p /var/www/html
RUN ln -s /var/www/html /html
VOLUME /html

RUN mkdir -p /var/www/js
RUN ln -s /var/www/js /js
VOLUME /js

ADD vhost.txt /
RUN sudo cat vhost.txt >> /etc/apache2/sites-available/000-default.conf

ADD ports.txt /
RUN sudo cat ports.txt >> /etc/apache2/ports.conf

ADD *.md /html/
ADD *.html /html/
ADD *.js /js/
ADD *.html /js/

ADD *.sh /
RUN sudo chmod +x *.sh

RUN mkdir -p ~/repo/
RUN git clone https://github.com/kr-g/apache2-module-demo.git ~/repo/apache2-module-demo
RUN git clone https://github.com/mozilla/gecko-dev.git --single-branch --depth 1 --branch GECKO4502_2016040719_RELBRANCH ~/repo/gecko-dev

RUN mkdir -p ~/repo/spidermonkey
RUN ln -s ~/repo/spidermonkey /spidermonkey
VOLUME /spidermonkey

RUN /setup.sh

RUN cp ~/repo/gecko-dev/js/src/build_OPT.OBJ/js/src/shell/js ~/repo/spidermonkey
RUN ln -s ~/repo/spidermonkey/js ~/repo/spidermonkey/spidermonkeyjs 
RUN rm ~/repo/gecko-dev -rf


ENTRYPOINT /startup.sh


