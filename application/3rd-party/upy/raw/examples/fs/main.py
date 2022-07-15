import io
import log

log.info("II", "test")
f = io.open("abc.txt", "w")
print(f.write)
f.write("hello upy")
f.close()

