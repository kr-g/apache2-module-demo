FROM ubuntu:16.04

RUN apt-get update
RUN apt-get install -y vim curl unzip sudo markdown
RUN apt-get install -y git apache2 apache2-dev
RUN apt-get install -y autoconf2.13 python


EXPOSE 81
EXPOSE 82
EXPOSE 83


RUN mkdir -p /var/www/html && ln -s /var/www/html /html
VOLUME /html

RUN mkdir -p /var/www/js && ln -s /var/www/js /js
VOLUME /js

RUN mkdir -p /jsv8 && ln -s /var/www/jsv8 /jsv8
VOLUME /jsv8

ADD vhost.txt /
RUN cat vhost.txt >> /etc/apache2/sites-available/000-default.conf
ADD ports.txt /
RUN cat ports.txt >> /etc/apache2/ports.conf

ADD *.md /html/
ADD *.html /html/
ADD *.js /js/
ADD *.html /js/
ADD *.sh /
RUN chmod +x *.sh

RUN mkdir -p /repo/v8/out/x64.release/lib.target

RUN /get-v8.sh /
#RUN /setup-v8.sh /

ADD archive-v8.tar /repo/bin/v8/
ADD archive-v8.tar /repo/v8/out/x64.release/lib.target
RUN ldconfig /repo/v8/out/x64.release/lib.target

ADD demo-v8.tar /jsv8

RUN git clone https://github.com/kr-g/apache2-module-demo.git /repo/apache2-module-demo
RUN /setup.sh /

#RUN git clone https://github.com/mozilla/gecko-dev.git --single-branch --depth 1 --branch GECKO4502_2016040719_RELBRANCH /repo/gecko-dev
#ADD gecko-dev /repo/gecko-dev/
#RUN SHELL=$XTERM_SHELL /setup-spider.sh /
#RUN rm /repo/gecko-dev -rf

ADD js-a* /repo/bin/
RUN cat /repo/bin/js-a* > /repo/bin/spidermonkeyjs
RUN chmod +x /repo/bin/spidermonkeyjs
RUN ln -s /repo/bin/spidermonkeyjs /usr/local/sbin/spidermonkeyjs -f
RUN spidermonkeyjs --version



ENTRYPOINT sudo /startup.sh /




