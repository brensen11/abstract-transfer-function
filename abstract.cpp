#include <iostream>
#include <cmath>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/KnownBits.h>
#include <vector>
#include <stack>

using namespace llvm;
using std::cout;
using std::endl;

void print_known_bits(KnownBits kbs)
{
    std::stack<uint64_t> vals;

    for (int i = 0; i < kbs.getBitWidth(); i++)
    {
        APInt bit = kbs.Zero.extractBits(1, i);
        vals.push(bit.getLimitedValue());
    }

    std::cout << "Zero Bits: ";
    while (!vals.empty())
    {
        std::cout << vals.top();
        vals.pop();
    }
    std::cout << std::endl;

    for (int i = 0; i < kbs.getBitWidth(); i++)
    {
        APInt bit = kbs.One.extractBits(1, i);
        vals.push(bit.getLimitedValue());
    }
    std::cout << "One Bits: ";
    while (!vals.empty())
    {
        std::cout << vals.top();
        vals.pop();
    }
    std::cout << std::endl;
}

KnownBits int_to_known_bits(unsigned int integer, unsigned int bitwidth)
{
    KnownBits kb(bitwidth);
    kb.Zero.setAllBits();
    int n = integer;
    int pos = 0;

    while (n != 0)
    {
        int r = n % 3;
        switch (r)
        {
        case 0:
            // set Zero bit
            kb.Zero.setBit(pos);
            kb.One.clearBit(pos);
            break;
        case 1:
            // set One bit
            kb.One.setBit(pos);
            kb.Zero.clearBit(pos);
            break;
        case 2:
            // clear both
            kb.One.clearBit(pos);
            kb.Zero.clearBit(pos);
            break;
        }

        n = n / 3;
        pos++;
    }
    assert((kb.Zero & kb.One) == 0 && "KnownBits struct violated");
    return kb;
}

/*
write a function that, for a bitwidth that arrives as a function argument,
enumerates all possible valid abstract values at that bitwidth, for your chosen abstract domain.
*/
std::vector<KnownBits> enumerate_values(unsigned int bitwidth)
{
    std::vector<KnownBits> kbs;
    int possible_combinations = std::pow(3, bitwidth);

    for (int i = 0; i < possible_combinations; i++)
    {
        KnownBits bits = int_to_known_bits(i, bitwidth);
        kbs.push_back(bits);
    }
    return kbs;
}

/*
write a concretization function for your abstract domain.
it takes an abstract value and returns a set of APInts that are in the concretization set for that abstract value
*/
std::vector<APInt> concretize_known_bits(KnownBits kbs)
{
    std::vector<APInt> apints;
    int width = kbs.getBitWidth();
    int possible_ints = 1;
    for (int bit = 0; bit < width; bit++)
    {
        if (kbs.One.extractBits(1, bit) == 0 && kbs.Zero.extractBits(1, bit) == 0)
        {
            possible_ints *= 2; // both 0 is top, which would multiply possibly ints by 2
        }
    }

    // we're basically counting up in binary only using the bits that are T
    for (int c = 0; c < possible_ints; c++)
    {
        int value = 0;
        int pow = 1;
        int div = c;
        int rem;

        for (int b = 0; b < width; b++)
        {
            if (kbs.Zero.extractBits(1, b) == 1)
            {
                // skip basically.
            }
            else if (kbs.One.extractBits(1, b) == 1)
            {
                value += pow;
            }
            else
            { // both == 0
                // use c to calculate
                rem = div % 2;
                if (rem == 1)
                    value += pow;
                div = div / 2;
            }
            pow *= 2;
        }
        APInt apint(width, value);
        apints.push_back(apint);
    }
    return apints;
}

APInt concrete_op(APInt val1, APInt val2)
{
    uint64_t shiftAmount = val2.getZExtValue();
    if (shiftAmount >= val1.getBitWidth())
    {
        return APInt(val1.getBitWidth(), 0);
    }
    return val1.shl(shiftAmount);
}

/*
write an abstraction function for your abstract domain.
it takes a set of APInts and returns the best abstract value whose concretization set includes the given set.
*/
KnownBits abstract_known_bits(std::vector<APInt> values)
{
    // two APInts and then all 1's bottom up transfer node lattice using 'and' for each
    APInt One(values[0].getBitWidth(), 0);
    APInt Zero(values[0].getBitWidth(), 0);
    One.setAllBits();
    Zero.setAllBits();

    for (APInt value : values)
    {
        One = One & value;
        APInt notValue = ~value;
        Zero = Zero & notValue;
    }
    KnownBits res;
    res.One = One;
    res.Zero = Zero;
    return res;
}

KnownBits combine_known_bits(std::vector<KnownBits> kbs)
{
    APInt One(kbs[0].getBitWidth(), 0);
    APInt Zero(kbs[0].getBitWidth(), 0);
    One.setAllBits();
    Zero.setAllBits();

    for (KnownBits value : kbs)
    {
        One = One & value.One;
        Zero = Zero & value.Zero;
    }
    KnownBits res;
    res.One = One;
    res.Zero = Zero;
    return res;
}

bool more_precise(KnownBits k1, KnownBits k2)
{
    return ((k1.One & k2.One) != k1.One) && ((k1.Zero & k2.Zero) != k1.Zero);
}

unsigned int count_llvm_more_precise(std::vector<KnownBits> llvm_results, std::vector<KnownBits> my_results)
{
    unsigned int llvm_more_precise = 0;
    for (int i = 0; i < llvm_results.size(); i++)
    {
        if (more_precise(llvm_results[i], my_results[i]))
        {
            llvm_more_precise++;
        }
    }
    return llvm_more_precise;
}

unsigned int count_composite_more_precise(std::vector<KnownBits> llvm_results, std::vector<KnownBits> my_results)
{
    unsigned int composite_more_precise = 0;
    for (int i = 0; i < my_results.size(); i++)
    {
        if (more_precise(my_results[i], llvm_results[i]))
        {
            composite_more_precise++;
        }
    }
    return composite_more_precise;
}

/*
write a function that exhaustively tests the two transfer functions
(the composite one from LLVM, and also the decomposed one using transfer functions for simpler instructions)
against each other. report the total number of abstract values, the total number of times the composite transfer function is more precise,
the total number of times the decomposed transfer function is more precise,
and finally the number of times the two transfer functions return incomparable results.
of course, we expect this last number to be zero, but you never know.
*/
int main()
{
    int bitwidth = 4;
    std::vector<KnownBits> kbs = enumerate_values(bitwidth); // all known bit variations of this bitwidth
    cout << "Number of abstract values: " << kbs.size() << endl;

    std::vector<KnownBits> llvm_results;
    std::vector<KnownBits> my_results;

    for (int i = 0; i < kbs.size(); i++)
    {
        for (int j = 0; j < kbs.size(); j++)
        {
            KnownBits kb1 = kbs[i];
            KnownBits kb2 = kbs[j];

            // do for llvm version
            KnownBits llvm_res = KnownBits::shl(kb1, kb2);
            llvm_results.push_back(llvm_res);

            // naive version
            std::vector<APInt> c = concretize_known_bits(kb1);
            std::vector<APInt> c2 = concretize_known_bits(kb2);
            c.insert(c.end(), c2.begin(), c2.end());

            std::vector<APInt> c_res;
            for (int i = 0; i < c.size(); i++)
            {
                for (int j = 0; j < c.size(); j++)
                {
                    c_res.push_back(concrete_op(c[i], c[j]));
                }
            }
            my_results.push_back(abstract_known_bits(c_res));
        }
    }

    // cout << "My Known Bits:" << endl;
    assert(my_results.size() == llvm_results.size());

    cout << "Number of times the composite transfer function is more precise:";
    cout << count_composite_more_precise(llvm_results, my_results) << endl;

    cout << "Number of times the llvm transfer function is more precise:";
    cout << count_llvm_more_precise(llvm_results, my_results) << endl;
    // for (auto kbs : my_results)
    // {
    //     print_known_bits(kbs);
    // }
    // cout << endl;

    // cout << "llvm known bits" << endl;
    // for (auto kbs : llvm_results) {
    //     print_known_bits(kbs);
    // }
}