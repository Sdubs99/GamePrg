#include <stdio.h>
#include <math.h>
#include <windows.h>

// 음계에 해당하는 index 정의 (도: 0, 레: 2, 미: 4, 파: 5, 솔: 7, 라: 9, 시: 11, 도: 12)
#define C  0  // 도
#define D  2  // 레
#define E  4  // 미
#define F  5  // 파
#define G  7  // 솔
#define A  9  // 라

int calc_frequency(int octave, int inx);

int main(void)
{
    
    int melody[] = {
        G, G, A, A, G, G, E,  
        G, G, E, E, D,        
        G, G, A, A, G, G, E,  
        G, E, D, E, C         
    };

    
    int duration[] = {
        400, 400, 400, 400, 400, 400, 800,
        400, 400, 400, 400, 800,
        400, 400, 400, 400, 400, 400, 800,
        400, 400, 400, 400, 800
    };

    int total_notes = sizeof(melody) / sizeof(melody[0]);
    int i;

    printf("School Bell Melody Start...\n");

    for (i = 0; i < total_notes; i++)
    {
        
        int freq = calc_frequency(4, melody[i]);

        
        Beep(freq, duration[i]);

       
        Sleep(50);
    }

    printf("연주가 완료되었습니다.\n");
    return 0;
}

int calc_frequency(int octave, int inx)
{
    double do_scale = 32.7032;
    double ratio = pow(2., 1 / 12.), temp;
    int i;

    temp = do_scale * pow(2, octave - 1);
    for (i = 0; i < inx; i++)
    {
        temp = (int)(temp + 0.5);
        temp *= ratio;
    }
    return (int)temp;
}
