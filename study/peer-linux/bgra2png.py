import struct,zlib,sys
src,dst=sys.argv[1],sys.argv[2]; W=int(sys.argv[3]); H=int(sys.argv[4]); S=2
d=open(src,'rb').read()
rows=[]
for y in range(H):
    line=bytearray()
    for x in range(W):
        o=(y*W+x)*4; b,g,r=d[o],d[o+1],d[o+2]
        line += bytes((r,g,b))*S
    for _ in range(S): rows.append(b'\x00'+bytes(line))
def ch(t,dd): return struct.pack('>I',len(dd))+t+dd+struct.pack('>I',zlib.crc32(t+dd)&0xffffffff)
open(dst,'wb').write(b'\x89PNG\r\n\x1a\n'+ch(b'IHDR',struct.pack('>IIBBBBB',W*S,H*S,8,2,0,0,0))+ch(b'IDAT',zlib.compress(b''.join(rows),6))+ch(b'IEND',b''))
print(dst)
