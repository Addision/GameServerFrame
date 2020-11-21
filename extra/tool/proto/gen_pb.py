'''
Author: jia.lai
Date: 2020-09-09 14:15:36
LastEditTime: 2020-09-09 15:32:24
Description: 生成pb文件工具
Version: 1.0
'''
import os
import sys


def gen_pb(gen_type, proto_path, proto_file, pb_path):
    gen_cmd = "protoc -I=" + proto_path + " "
    if gen_type == 'cpp':
        gen_cmd += "--cpp_out=" + pb_path + " " + proto_file
        pass
    if gen_type == 'cs':
        gen_cmd += "--csharp_out=" + pb_path + " " + proto_file
        pass
    if gen_type == 'lua':
        gen_cmd += "--lua_out=" + pb_path + " " + proto_file
        pass
    if gen_type == 'py':
        gen_cmd += "--python_out=" + pb_path + " " + proto_file
        pass
    print(gen_cmd)
    os.popen(gen_cmd)
    pass


def gen(gen_type, proto_path, pb_path):
    if not proto_path:
        return
    list_dirs = os.listdir(proto_path)
    for proto_file in list_dirs:
        if not proto_file.endswith((".proto")):
            continue
        gen_pb(gen_type, proto_path, proto_file, pb_path)


if __name__ == '__main__':
    # print(sys.argv)
    try:
        gen(sys.argv[1], sys.argv[2], sys.argv[3])
    except:
        os.system("pause")
    pass
