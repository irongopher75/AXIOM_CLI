#include "../core/types.hpp"
#include <iostream>
#include <cassert>

using namespace Axiom;

void test_price_arithmetic() {
    Price p1(1.00);
    Price p2(0.05);
    Price sum = p1 + p2;
    assert(sum.get_raw() == 10500);
    assert(sum.str() == "1.05");
    
    Price p3(271.05);
    assert(p3.get_raw() == 2710500);
    
    std::cout << "✅ Price fixed-point tests passed!" << std::endl;
}

int main() {
    test_price_arithmetic();
    return 0;
}
