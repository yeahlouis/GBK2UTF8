# -*- coding: UTF-8 -*-
def gbkutf82Unicode(num):
    if (num <= 0):
        return 0
    elif (num > 0 and num <= 0x7F):
        return num
    if (num >= 0x10000):
        a = (num >> 16) & 0x0F
        b = (num >> 8) & 0x3F
        c = num & 0x3F
        return c | (b << 6) | (a <<12)
    elif (num >= 0x100):
        a = (num >> 8) & 0x1F
        b = num & 0x3F
        return b | a << 6
    return 0;     

def main():
    filename = 'gen_gbk2uni.h'
    lgbk=[]
    lu16=[]
    i = 0
    j = 0
    for b1 in range(0x81, 0xFF):
        for b2 in range(0x40, 0xFF + 1):
            i += 1
            b = bytes([b1, b2])
            n = int.from_bytes(b, byteorder='big', signed=False)
            try:
                c = b.decode('GBK')
                d = c.encode('utf-8');
                u8 = int.from_bytes(d, byteorder='big', signed=False)
                if (len(d) > 3): #GBK编码len(d)都小于等于3,这样才能在Unicode编码中
                    print("=========error!===========")
                u16 = gbkutf82Unicode(u8)
                lgbk.append(hex(n).upper())
                s = hex(u16).upper()
                if (len(s) < 6) :
                    n0 = '000000'
                    nd = 6 - len(s)
                    s = s[0:2] + n0[0:nd] + s[2:]
                lu16.append(s)
            except UnicodeDecodeError:
                lgbk.append(hex(n).upper())
                lu16.append('0x0001'.upper())
                j += 1
                continue

    print("count = [" + str(i) + "]")
    print("count = [" + str(j) + "]")
    print("count = [" + str(i - j) + "]")

    print("len(lgbk) = " + str(len(lgbk)))
    print("len(lu16) = " + str(len(lu16)))

    with open(filename, 'w') as f:
        i = 0
        for l in lgbk:
            i += 1
            f.write(l)
            f.write(',')
            if (i%16 == 0):
                f.write('\n')
        f.write('\n\n//=====================\n')
        f.write('//=====================\n')
        f.write('//=====================\n\n\n')
        i = 0
        for l in lu16:
            i += 1
            f.write(l)
            f.write(',')
            if (i%16 == 0):
                f.write('\n')


    #b'\xce\xd2'
    b = b'\xce\xd2'
    print(b.decode('GBK'))
    print(lgbk[(0xce - 0x81) * 12 * 16 + 0xd2 - 0x40])
    print(lu16[(0xce - 0x81) * 12 * 16 + 0xd2 - 0x40])
    print("================result==================")

if __name__ == "__main__":
    # execute only if run as a script
    print("\n================start+++================\n")
    main()
    print("\n================end-----================\n")
    #print("\n")
