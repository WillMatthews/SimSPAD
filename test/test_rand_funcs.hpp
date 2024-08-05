/*
// Sanity check random number generation as used in the program
void SiPM::test_rand_funcs()
{
    const int numStars = 100;    // maximum number of stars to distribute
    const int numIntervals = 10; // number of intervals
    int iters = 10000;

    cout << "\n\n***** UNIFORM *****\n"
         << endl;
    double s1;
    double test1 = 0;
    int p1[numIntervals] = {};
    for (int i = 0; i < iters; i++)
    {
        s1 = unif_rand_double(0, 1);
        test1 += s1;
        ++p1[int(numIntervals * s1)];
    }
    cout << "mean dist 1: " << test1 / iters << endl;

    for (int i = 0; i < numIntervals; i++)
    {
        cout << float(i) / numIntervals << "-" << float(i + 1) / numIntervals << "\t: ";
        cout << string(p1[i] * numStars / iters, '*') << endl;
    }

    double lambda = 3.5;
    const int poissonNumIntervals = 20;
    int p2[poissonNumIntervals] = {};
    int s2;
    for (int i = 0; i < iters; i++)
    {
        poisson_distribution<int> distribution(lambda);
        s2 = distribution(poissonEngine);
        ++p2[int(s2)];
    }

    cout << "\n\n***** POISSON *****\n"
         << endl;
    for (int i = 0; i < poissonNumIntervals; i++)
    {
        cout << i << "\t: ";
        cout << string(p2[i] * numStars / iters, '*') << endl;
    }
}
*/