#include <stdio.h>
#include <stdlib.h>
#include "upy.h"


int main(int argc, char *argv[])
{
    upy_t *e = upy_init(1000 * sizeof(upy_val_t));
    upy_val_t res = upy_parse(e, argv[1]);

    if( upy_try(e) ) {
        upy_val_t error = upy_pop(e);
        if( upy_is_string(error) ){
            printf("SyntaxError: %s\r\n", upy_2_string(error));
        }
        return 0;
    }

    upy_call(e, res, res, 0, NULL);
}
