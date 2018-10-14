#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION // _____ Must be in this order
#include "stb_image/stb_image.h" // __|

int main(int argc, char* argv[])
{
    int x;
    int y;
    int n;
    unsigned char * data;
    
    data = stbi_load("testimg.jpg", &x, &y, &n, 0);
    
    /*for (int i = 0; i < y; i++)
    {
        for (int j = 0; j < x; j++)
        {
            printf("(");
            
            for (int k = 0; k < n; k++)
            {
                printf("%i ", (int) data[i][j][k]);
            }
            
            printf(") ");
        }
        
        printf("\n");
    }*/
    
    for (int i = 0; i < (x * y * n); i++)
    {
        printf("%03d ", data[i]);
        
        if (!(i % 50)) {
            printf("\n");
        }
    }
    
    stbi_image_free(data);
    
    return 0;
}
