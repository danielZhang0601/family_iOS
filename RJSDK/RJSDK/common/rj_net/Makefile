#
# �����ִ���ļ�, ��̬��, ��̬��ģ��
#
# Ĭ��ʹ��linux32�汾���� : make
# ������뺣˼arm�汾��Ϊ : make plat=hisi
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


#ϵͳ����
SHELL = /bin/bash
DIR_CUR = $(shell pwd)


#��Ŀ¼������Ҫ�����.c,.cpp�ļ�
DIRS = src conn uv_socket

#�Ż�ѡ�� ʾ��: -fPIC -g
CFLAGS =

#�û��Զ������ѡ��. ʾ��: -D__RJ_LINUX32__
CXXFLAGS = -D$(platform)

#���ؿ�ѡ��. ʾ��: -Wl,-O1
LDFLAGS =

#����ͷ�ļ�. ʾ��: -I ../include -I $(DIR_CUR)/include
BASC_INC =  -I ../../include/libuv -I ../../include/rj_base  
INCLUDES += $(BASC_INC) -I ../../include/rj_net   -I ./conn  -I ./src   -I ./websocket

#���õľ�̬��,�Լ���̬��Ŀ¼. ʾ��: -L ../lib_linux -luv
DIR_STATIC = -L $(libpath)
LIB_STATIC = -Bstatic -luv -lrj_base

#���õĶ�̬��,�Լ���̬��Ŀ¼. ʾ��: -L ../lib_linux -lrt -lpthread
DIR_DYNAMIC = -L $(libpath)
LIB_DYNAMIC = -Bdynamic -lrt -lpthread


#����Ŀ������
TARGET_PATH = $(path)
TARGET_NAME = rj_net


#����Ŀ��
TARGET_LIB := $(TARGET_PATH)/lib$(TARGET_NAME).a
TARGET_SO  := $(TARGET_PATH)/$(TARGET_NAME).so
TARGET_EXE := $(TARGET_PATH)/$(TARGET_NAME)

ALL_TARGET += $(TARGET_LIB)



#########################################################
## ������Ϣ���޸�
#########################################################

#������
CXX = $(cxx)
CAR = $(car)
CRANLIB = $(cranlib)
CVERSION = $(version)

RM = -rm -f


#�����汾����
ifeq ($(CVERSION), release)
	
else
	CFLAGS += -g
endif


#�����ļ�
FIND_FILES_CPP = $(wildcard $(DIR_CUR)/$(dir)/*.cpp)
FIND_FILES_C = $(wildcard $(DIR_CUR)/$(dir)/*.c)

SOURCES = $(foreach dir, $(DIRS), $(FIND_FILES_C))
SOURCES += $(foreach dir, $(DIRS), $(FIND_FILES_CPP))

LIB_L_OBJS = $(addsuffix .ol, $(SOURCES))
LIB_SO_OBJS = $(addsuffix .os, $(SOURCES))

L_CFLAGS = $(CFLAGS)
SO_CFLAGS = -shared $(CFLAGS)


#���ϲ���
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