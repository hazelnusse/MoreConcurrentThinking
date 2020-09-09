#include <foo.hpp>
#include <boost/ut.hpp>

int main()
{
    using namespace boost::ut;
    expect(foo() == 42_i);
}
