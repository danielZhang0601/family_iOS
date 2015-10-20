#
# 编译可执行文件, 动态库, 静态库模板
#
# 默认使用linux32版本编译 : make
# 如果编译海思arm版本则为 : make plat=hisi
#

# debug release
version=debug

# linux, hisi
plat=linux


ifeq ($(plat), linux)
	cxx:=g++
	car:=ar
	cranlib:=ranlib
	platform:=__RJ_LINUX32__
	libpath:=../../lib/lib_linux
	path:=../../lib/lib_linux
else
	cxx:=arm-hisiv100nptl-linux-g++
	car:=arm-hisiv100nptl-linux-ar
	cranlib:=arm-hisiv100nptl-linux-ranlib
	platform:=__RJ_LINUX32_ARM__
	libpath:=../../lib/lib_arm
	path:=../../lib/lib_arm
endif


#系统参数
SHELL = /bin/bash
DIR_CUR = $(shell pwd)


#从目录检索需要编译的.c,.cpp文件
DIRS = pub src

#优化选项 示例: -fPIC -g
CFLAGS =

#用户自定义编译选项. 示例: -D__RJ_LINUX32__
CXXFLAGS = -D$(platform)

#加载库选项. 示例: -Wl,-O1
LDFLAGS =

#引用头文件. 示例: -I ../include -I $(DIR_CUR)/include
BASC_INC =   -I ../../include/rj_base  
INCLUDES += $(BASC_INC)  

#引用的静态库,以及静态库目录. 示例: -L ../lib_linux -luv
DIR_STATIC = -L $(libpath)
LIB_STATIC = -Bstatic

#引用的动态库,以及动态库目录. 示例: -L ../lib_linux -lrt -lpthread
DIR_DYNAMIC = -L $(libpath)
LIB_DYNAMIC = -Bdynamic -lrt -lpthread


#编译目标名称
TARGET_PATH = $(path)
TARGET_NAME = rj_base


#编译目标
TARGET_LIB := $(TARGET_PATH)/lib$(TARGET_NAME).a
TARGET_SO  := $(TARGET_PATH)/$(TARGET_NAME).so
TARGET_EXE := $(TARGET_PATH)/$(TARGET_NAME)

ALL_TARGET += $(TARGET_LIB)



#########################################################
## 以下信息勿修改
#########################################################

#编译器
CXX = $(cxx)
CAR = $(car)
CRANLIB = $(cranlib)
CVERSION = $(version)

RM = -rm -f


#处理版本问题
ifeq ($(CVERSION), release)
	
else
	CFLAGS += -g
endif


#编译文件
FIND_FILES_CPP = $(wildcard $(DIR_CUR)/$(dir)/*.cpp)
FIND_FILES_C = $(wildcard $(DIR_CUR)/$(dir)/*.c)

SOURCES = $(foreach dir, $(DIRS), $(FIND_FILES_C))
SOURCES += $(foreach dir, $(DIRS), $(FIND_FILES_CPP))

LIB_L_OBJS = $(addsuffix .ol, $(SOURCES))
LIB_SO_OBJS = $(addsuffix .os, $(SOURCES))

L_CFLAGS = $(CFLAGS)
SO_CFLAGS = -shared $(CFLAGS)


#整合参数
L_PARAMS =  $(L_CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(DIR_STATIC) $(LIB_STATIC) $(DIR_DYNAMIC) $(LIB_DYNAMIC) $(INCLUDES)
SO_PARAMS = $(SO_CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(DIR_STATIC) $(LIB_STATIC) $(DIR_DYNAMIC) $(LIB_DYNAMIC) $(INCLUDES)


.PHONY:all lib so exe clean tip_all

all: tip_all $(ALL_TARGET)
lib: tip_all $(TARGET_LIB)
so:	 tip_all $(TARGET_SO)
exe: tip_all $(TARGET_EXE)


%.c.ol: %.c
	$(CXX) $(L_PARAMS) -c -o $@ $<

%.cpp.ol: %.cpp
	$(CXX) $(L_PARAMS) -c -o $@ $<

%.c.os: %.c
	$(CXX) $(SO_PARAMS) -c -o $@ $<

%.cpp.os: %.cpp
	$(CXX) $(SO_PARAMS) -c -o $@ $<


$(TARGET_LIB): $(LIB_L_OBJS)
	$(echo_tip_lib)
	$(CAR) rs $(TARGET_LIB) $^
	$(CRANLIB) $(TARGET_LIB)

$(TARGET_SO): $(LIB_SO_OBJS)
	$(echo_tip_so)
	$(CXX) $(SO_PARAMS) -o $@ $(LIB_SO_OBJS)

$(TARGET_EXE): $(LIB_L_OBJS)
	$(echo_tip_exe)
	$(CXX) -o $@ $(LIB_L_OBJS) $(L_PARAMS)

clean:
	@echo "++++++++++++++++ make clean ++++++++++++++"
	@echo "++ RM=" $(RM)
	
	$(RM) $(LIB_L_OBJS)
	$(RM) $(LIB_SO_OBJS)

	$(RM) $(TARGET_LIB)
	$(RM) $(TARGET_SO)
	$(RM) $(TARGET_EXE)

	@echo "++++++++++++++++++++++++++++++++++++++++++"


tip_all:
	@echo "++++++++++++++++ make param ++++++++++++++"
	@echo "++ DIR_CUR=" $(DIR_CUR)
	@echo "++ CXX=" $(CXX)
	@echo "++ CAR=" $(CAR)
	@echo "++ CRANLIB=" $(CRANLIB)
	@echo "++ CVERSION=" $(CVERSION)
	@echo "++ DIRS=" $(DIRS)
	@echo "++ CFLAGS=" $(CFLAGS)
	@echo "++ CXXFLAGS=" $(CXXFLAGS)
	@echo "++ LDFLAGS=" $(LDFLAGS)
	@echo "++ INCLUDES=" $(INCLUDES)
	@echo "++ DIR_STATIC=" $(DIR_STATIC)
	@echo "++ LIB_STATIC=" $(LIB_STATIC)
	@echo "++ DIR_DYNAMIC=" $(DIR_DYNAMIC)
	@echo "++ LIB_DYNAMIC=" $(LIB_DYNAMIC)
	@echo "++ SOURCES=" $(notdir $(SOURCES))
	@echo "++++++++++++++++++++++++++++++++++++++++++"

define echo_tip_lib
	@echo "++++++++++++++++ make *.a ++++++++++++++++"
	@echo "++ L_CFLAGS=" $(L_CFLAGS)
	@echo "++ LIB_L_OBJS=" $(notdir $(LIB_L_OBJS))
	@echo "++ L_PARAMS=" $(L_PARAMS)
	@echo "++ TARGET_LIB=" $(TARGET_LIB)
	@echo "++++++++++++++++++++++++++++++++++++++++++"
endef

define echo_tip_so
	@echo "++++++++++++++++ make *.so +++++++++++++++"
	@echo "++ SO_CFLAGS=" $(SO_CFLAGS)
	@echo "++ LIB_SO_OBJS=" $(notdir $(LIB_SO_OBJS))
	@echo "++ SO_PARAMS=" $(SO_PARAMS)
	@echo "++ TARGET_SO=" $(TARGET_SO)
	@echo "++++++++++++++++++++++++++++++++++++++++++"
endef

define echo_tip_exe
	@echo "++++++++++++++++ make exe ++++++++++++++++"
	@echo "++ L_CFLAGS=" $(L_CFLAGS)
	@echo "++ LIB_L_OBJS=" $(notdir $(LIB_L_OBJS))
	@echo "++ L_PARAMS=" $(L_PARAMS)
	@echo "++ TARGET_EXE=" $(TARGET_EXE)
	@echo "++++++++++++++++++++++++++++++++++++++++++"
endef
