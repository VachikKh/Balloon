#include <stdbool.h>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <unistd.h>
// libs for anomaly 
#include <cmath>
#include <bits/stdc++.h>
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
double acc_list[10] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
unsigned int curr_acc_ind = 0;

bool is_burst(double altitude, double acc)
{
    acc_list[curr_acc_ind++ % 10] = acc;
    int n = 10; // the lenght of an array
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
    double copied[10];
    int loop;

    for (loop = 0; loop < 10; loop++)
    {
        copied[loop] = acc_list[loop];
    }
    array_sort(copied, n);

    // Now pass the sorted array to calculate
    // the median of your array.
    double curr_acc = find_median(acc_list, n);
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
            if (stop - start > 5000)
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
{ // if ballon goes up from 7000 m then comes down to 5000 m than the parachute is realesed

    // # condition for descending open up the parachute
    if (parachute_engage && altitude <= 5000)
    {
        printf("parachute is open now \n");
        parachute_rel = true;
    }
    // # check if you once passed 6000 metre means you are ascending
    if (not parachute_engage && altitude >= 7000)
    {
        printf("you have passed 7000 m, parachute is closed \n");
        parachute_engage = true;
    }
    // # additional condition
    if (burst && altitude <= 5000)
        parachute_rel = true;

    return parachute_rel;
}

int main()
{
    //// altitude anomaly detection
    // double altitudes[3] = {8.0, 9.0, 105.0}; // TODO replace with real altitude data
    // vector<int> annomaly_points;

    // annomaly_points.clear();
    // annomaly_points = annomalyPoints(altitudes);
    // double annomaly_points_arr[6];
    // for (int i = 0; i < 6; i++)
    //     annomaly_points_arr[i] = annomaly_points[i];

    // int n = sizeof(annomaly_points_arr) / sizeof(annomaly_points_arr[0]);
    // int annomaly_ind = mostFrequent(annomaly_points_arr, n);
    // altitudes[annomaly_ind] = 0;
    // double altitude = (altitudes[0] + altitudes[1] + altitudes[2]) / 2;
    double altitude = 1000;
    printf("altitude without annomaly %f \n", altitude);

    //////////////////////////// logic part is here /////////////////////
    bool camer_charge = true;
    bool ballon_sep = false;
    bool parachute_rel = false;
    bool beep_beep = false;

    // testing by mock variables 
    // #TODO these values should be replaced by real data of accelerarion and altitude
    int acc_mock[32] = {5, 5, 5, 5, 5, 4, 4, 4, 3, 3, 3, 4, 9, 9, 9, 9, 9, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    double alt_mock[5] = {1000, 8000, 20000, 5000, 2000};
    for (int i = 0; i < 33; i++)
    {
        sleep(1);
        printf("**\n");
        if (not burst)
            burst = is_burst(alt_mock[i], acc_mock[i]);
        parachute_relief(alt_mock[i], burst);
        // printf(f"altitude:{alt}\n");

        printf("altitude: %f \n", alt_mock[i]);
        // printf("time %lu \n", millis());
        if (parachute_relief(alt_mock[i], burst))
            break;
    }

    return 0;
}
