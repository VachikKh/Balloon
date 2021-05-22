#include <stdbool.h>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <unistd.h>

using namespace std;


// Function to sort an array to find the median of an array
void array_sort(double *array, int n)
{
    // sorts array ascending way
    // declare some local variables
    int i = 0, j = 0, temp = 0;

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n - 1; j++)
        {
            if (array[j] > array[j + 1])
            {
                temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }

}

unsigned long millis()
{
    //for measuring time
    auto time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
}

// function to calculate the median of the array
float find_median(double array[], int n)
{
    float median = 0;

    // if number of elements are even
    if (n % 2 == 0)
        median = (array[(n - 1) / 2] + array[n / 2]) / 2.0;
    // if number of elements are odd
    else
        median = array[n / 2];

    return median;
}

bool burst = false;
bool free_fall = false;
unsigned long start = 0;
unsigned long stop = 0;
int counter = 0;
double prev_acc = -1;
// the array we want to find the median
double acc_list[3] = {10, 10, 10};
double altitude_list[3] = {0, 0, 0};

unsigned int curr_acc_ind = 0;
unsigned int curr_alt_ind = 0;


bool is_burst(double altitude, double acc)
{
    int n = 3; // the lenght of an array
    acc_list[curr_acc_ind++%3] = acc;
    float median = 0;

    // ##############################
    // # if the ballon goes too high#
    // ##############################
    if (altitude >= 33333)
    {
        printf("balloon released \n");
        burst = true;
        return burst;
    }

    // Sort the array in ascending order
    double copied_acc[3];
    int loop = 0;
    for (loop = 0; loop < 3; loop++)
    {
        copied_acc[loop] = acc_list[loop];
    }
    array_sort(copied_acc, n);

    // Now pass the sorted array to calculate
    // the median of your array.
    double curr_acc = find_median(copied_acc, n);
    printf("curr acc = %f  prev acc = %f", curr_acc, prev_acc);
    if ((0 <= curr_acc && curr_acc <= 4) && (0 <= prev_acc && prev_acc <= 4))
    {
        prev_acc = curr_acc;

        if (false == free_fall)
        {
            start = millis();
            free_fall = true;
        }

        else if (free_fall)
        {
            stop = millis();
            //  if 1 sec continious free falling
            printf("time diff: %lu \n", stop - start);
            if (stop - start > 1000)
            {

                printf("descending 1 second in a row of \n");

                burst = true;
            }
        }
    }
    else
    {
        printf("not right acc condition----------------------- \n");
        prev_acc = curr_acc;
        free_fall = false;
    }
    printf("free fall %d \n", free_fall);
    return burst;
}

//  parachute realese logic
bool parachute_engage = false;
bool parachute_rel = false;

bool parachute_relief(double altitude, bool burst)
{ // if ballon goes up from 6000 m then comes down to 5000 m than the parachute is realesed

    // # condition for descending open up the parachute
    printf("curr altitude: %f \n", altitude);
    if (parachute_engage && (altitude <= 5000))
    {
        printf("parachute is open now \n");
        parachute_rel = true;
    }
    // # check if you once passed 6000 metre means you are ascending
    if (not parachute_engage && altitude >= 6000)
    {
        printf("you have passed 6000 m, parachute is closed \n");
        parachute_engage = true;
    }
    // # additional condition
    if (burst && altitude <= 5000)
        parachute_rel = true;

    return parachute_rel;
}

int main()
{
    // double altitude = 1000;
    // printf("altitude without annomaly %f \n", altitude);

    //////////////////////////// logic part is here /////////////////////
    bool camer_charge = true;
    bool ballon_sep = false;
    bool parachute_rel = false;
    bool beep_beep = false;

    // testing by mock variables 
    // #TODO these values should be replaced by real data of accelerarion and altitude
    double acc_mock[38] = {9.8, 9.8, 9.8, 9.8, 9.8, 9.8, 9.8, 9.8, 9.8, 9.8, 9.8, 7, 11, 2, 1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,1, 9.8, 9.8, 9.4, 9.5, 9.5};
    double alt_mock[38] = {50, 100, 200, 1000, 2000, 3000, 4000, 5000, 6000, 9000, 10000, 12000, 18000, 15000, 12000, 11000, 10000, 8000, 7000, 5000, 4000, 2000, 1000, 500, 200, 100, 50, 40, 30, 20, 10, 9, 8, 7, 6, 5, 4};
    for (int i = 0; i < 33; i++)
    {
        sleep(1);
        ////////////////////// accumulating altitude list //////////////
        printf("**\n");
        altitude_list[curr_alt_ind++%3] = alt_mock[i];
        int n = 3; // the lenght of an array
        double copied_alt[3];
        int loop;

        for (loop = 0; loop < 3; loop++)
        {
            copied_alt[loop] = altitude_list[loop];
        }

        array_sort(copied_alt, n);
        // Now pass the sorted array to calculate
        // the median of your array.
        double curr_alt = find_median(copied_alt, n);

        //////////////////////////////////////////////////

        if (not burst)
            burst = is_burst(curr_alt, acc_mock[i]);
        printf("burst %d : ", burst);

        parachute_relief(curr_alt, burst);
        // printf(f"altitude:{alt}\n");

      
        // printf("time %lu \n", millis());
        if (parachute_relief(curr_alt, burst))
            break;
    }

    return 0;
}
