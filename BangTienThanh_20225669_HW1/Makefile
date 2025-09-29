# Tên trình biên dịch
CC = gcc

# Cờ biên dịch
CFLAGS = -std=c11 -Wall -Wextra -g

# Tên file thực thi
TARGET = main.exe

# Các file nguồn
SRCS = main.c utils.c

# File đối tượng (tự động chuyển từ .c sang .o)
OBJS = $(SRCS:.c=.o)

# Luật mặc định
all: $(TARGET)

# Luật tạo file thực thi
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Luật tạo file .o từ .c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Luật dọn dẹp
clean:
	rm -f $(OBJS) $(TARGET)
