FROM node:14
LABEL maintainer="Carlos Nunez <dev@carlosnunez.me>"

COPY . /app
WORKDIR /app
RUN npm ci && npm run build && npm -g install
ENTRYPOINT [ "gitlab-search" ]
