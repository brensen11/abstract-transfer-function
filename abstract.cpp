#include <iostream>
#include <cmath>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/KnownBits.h>
#include <vector>
#include <stack>

using namespace llvm;

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
        apints.push_back(APInt(width, value));
    }
    return apints;
}

unsigned int concrete_op(APInt val) {
    return val.countLeadingOnes();
}

/*
write an abstraction function for your abstract domain.
it takes a set of APInts and returns the best abstract value whose concretization set includes the given set.
*/
KnownBits abstract_known_bits(std::vector<unsigned int> values)
{
    // two APInts and then all 1's bottom up transfer node lattice using 'and' for each
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
    std::vector<KnownBits> kbs = enumerate_values(bitwidth);
    std::vector<APInt> cbs;
    for (KnownBits kb : kbs)
    {
        std::vector<APInt> c = concretize_known_bits(kb);
        cbs.insert(cbs.end(), c.begin(), c.end());
    }

    std::vector<unsigned int> results;
    // for each combo in cvs:
        // results.push_back(concrete_op(combo.first, combo.second));
    // abstract_known_bits(...) // after concrete_op
    //
    // auto vals = enumerate_values(2);
    // for (auto val : vals)
    // {
    //     print_known_bits(val);
    //     std::cout << std::endl;
    // }

    KnownBits kb(4);
    kb.Zero.setBit(0);
    print_known_bits(kb);

    auto pos_nums = concretize_known_bits(kb);
    for (APInt num : pos_nums)
    {
        std::cout << num.getLimitedValue() << std::endl;
    }
}