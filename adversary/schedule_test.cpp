
#include <bits/stdc++.h>
#include "schedules.hpp"
#include "adversarial_workload.hpp"

using namespace std;
using namespace churn;

int main()
{
    Rng rng(42);

    vector<Key> live = {40, 10, 70, 20, 50};

    UniformSchedule uniform;
    MinDeleteHighInsertSchedule minHigh;
    MaxDeleteLowInsertSchedule maxLow;
    ScapegoatStressSchedule scapegoat;
    AdversarialWorkload adversarial;

    size_t a = minHigh.pickDeleteIndex(live, rng);
    size_t b = maxLow.pickDeleteIndex(live, rng);

    cout << "Minimum deletion key: " << live[a] << endl;
    cout << "Maximum deletion key: " << live[b] << endl;

    Key h1 = minHigh.pickInsertKey(rng);
    Key h2 = minHigh.pickInsertKey(rng);

    Key l1 = maxLow.pickInsertKey(rng);
    Key l2 = maxLow.pickInsertKey(rng);

    cout << "Descending keys: " << h1 << " " << h2 << endl;
    cout << "Ascending keys: " << l1 << " " << l2 << endl;

    size_t u = uniform.pickDeleteIndex(live, rng);
    Key uk = uniform.pickInsertKey(rng);

    size_t s = scapegoat.pickDeleteIndex(live, rng);
    Key sk1 = scapegoat.pickInsertKey(rng);
    Key sk2 = scapegoat.pickInsertKey(rng);

    size_t d = adversarial.pickDeleteIndex(live, rng);
    Key dk1 = adversarial.pickInsertKey(rng);
    Key dk2 = adversarial.pickInsertKey(rng);

    bool passed = true;

    if (live[a] != 10 || live[b] != 70)
        passed = false;

    if (!(h1 > h2 && l1 < l2))
        passed = false;

    if (u >= live.size() || uk < 0 || uk >= kKeyUniverse)
        passed = false;

    if (s >= live.size() || !(sk1 < sk2))
        passed = false;

    if (live[d] != 10 || !(dk1 > dk2))
        passed = false;

    cout << "Uniform schedule: " << uniform.name() << endl;
    cout << "Min-high schedule: " << minHigh.name() << endl;
    cout << "Max-low schedule: " << maxLow.name() << endl;
    cout << "Scapegoat schedule: " << scapegoat.name() << endl;
    cout << "Adversarial schedule: " << adversarial.name() << endl;

    if (passed)
        cout << "\nAll schedule tests passed!" << endl;
    else
        cout << "\nSome schedule tests failed!" << endl;

    return passed ? 0 : 1;
}
