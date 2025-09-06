FROM gcc:12.2.0

WORKDIR /app

COPY runner.c .
RUN gcc -Wall -Wextra -O2 runner.c -o runner

# Создаем непривилегированного пользователя
RUN useradd -m runner && \
    chown -R runner:runner /app

USER runner

CMD ["./runner"]