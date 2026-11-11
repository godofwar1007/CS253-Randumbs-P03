#include <bits/stdc++.h>
#include <random>
using namespace std;

int main()
{
    int n;
    cin >> n;
    if(n<=0) cout<<"Invalid Input"<<endl;
    else
    {
    for(int i=1;i<=n;i++)
    {
        cout<<i<<" ";
    }
    cout<<endl;
    for(int i=n;i>=1;i--)
    {
        cout<<i<<" ";
    }
    cout<<endl;
    vector<int> a;
    for(int i=1;i<=n;i++) a.push_back(i);
    mt19937 rng(42);
    shuffle(a.begin(),a.end(),rng);
    for(int i=0;i<n;i++) cout<<a[i]<<" ";
    cout<<endl;
    }
    return 0;
}