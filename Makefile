# ------------------------------------------------------------
#  Makefile for EyeCare (MSVC NMake)
#
#  用法：
#    nmake            -> 默认编译 Release 版 (优化，静态链接)
#    nmake release    -> 编译 Release 版
#    nmake debug      -> 编译 Debug 版 (调试信息，无优化)
#    nmake clean      -> 清理所有生成的文件和目录
# ------------------------------------------------------------

CC = cl
RC = rc
LINK = link

# 基础编译/链接选项
CFLAGS_BASE  = /nologo /D "UNICODE" /D "_UNICODE" /utf-8
RCFLAGS      = /nologo
LDFLAGS_BASE = /nologo /SUBSYSTEM:WINDOWS
LIBS = user32.lib kernel32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib

# ------------------------------------------------
#  根据 BUILD 变量决定编译选项和输出目录
#    - 若未指定或为 release：优化，静态链接发布版运行时
#    - 若为 debug：调试信息，无优化，静态链接调试版运行时
# ------------------------------------------------
!IF "$(BUILD)" == "debug"
CFLAGS_EXTRA = /Od /Zi /MTd
LDFLAGS_EXTRA = /DEBUG
OUTDIR = debug
!ELSE
# 默认 release
CFLAGS_EXTRA = /O2 /MT
LDFLAGS_EXTRA =
OUTDIR = release
!ENDIF

# 拼接最终选项
CFLAGS = $(CFLAGS_BASE) $(CFLAGS_EXTRA)
LDFLAGS = $(LDFLAGS_BASE) $(LDFLAGS_EXTRA)

# 目标文件（位于 OUTDIR 内）
TARGET = $(OUTDIR)\EyeCare.exe
OBJS   = $(OUTDIR)\EyeCare.obj
RES    = $(OUTDIR)\EyeCare.res

# ------------------------------------------------
#  默认目标（all）
# ------------------------------------------------
all: $(TARGET)

# 链接
$(TARGET): $(OBJS) $(RES)
    $(LINK) $(LDFLAGS) $(OBJS) $(RES) /OUT:$(TARGET) $(LIBS)

# 编译 C++ 源文件，并确保输出目录存在
$(OBJS): EyeCare.cpp
    @if not exist $(OUTDIR) mkdir $(OUTDIR)
    $(CC) $(CFLAGS) /c EyeCare.cpp /Fo$(OBJS)

# 编译资源文件，并确保输出目录存在
$(RES): EyeCare.rc
    @if not exist $(OUTDIR) mkdir $(OUTDIR)
    $(RC) $(RCFLAGS) /fo$(RES) EyeCare.rc

# ------------------------------------------------
#  对外目标：release / debug
#  通过递归调用自身并传递 BUILD 变量
# ------------------------------------------------
release:
    $(MAKE) BUILD=release all

debug:
    $(MAKE) BUILD=debug all

# 清理：删除 release 和 debug 目录
clean:
    -if exist release rmdir /s /q release
    -if exist debug rmdir /s /q debug

# 防止与文件名冲突（无文件依赖）
.PHONY: all release debug clean