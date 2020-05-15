#include "shuntingyard.h"
#include "protocolsupport.h"
#include <math.h>


/*!
 * Replace "pi" or "e" in the string with the numeric values. This replacement
 * does not check to determine if "pi" or "e" are part of some other word.
 * \param input is the input string, which will be modified
 * \return a reference to input.
 */
std::string& ShuntingYard::replacePie(std::string& input)
{
    replaceinplace(input, "pi", "3.14159265358979323846");
    replaceinplace(input, "e" , "2.71828182845904523536");
    return input;

}// ShuntingYard::replacePie


/*!
 * Given a raw (untokenized) mathematical expression in infix notation, compute
 * the result. Allowable operators are " ( ) + - * / ^ ".
 * \param infix is the infix expresions to compute
 * \param ok is set to true if the computation is god. ok can point to NULL.
 * \return the computational result, or 0 if the computation cannot be performed.
 */
double ShuntingYard::computeInfix(const std::string& infix, bool* ok)
{
    bool inputok;
    std::string postfix = infixToPostfix(infix, &inputok);

    if(inputok)
        return computePostfix(postfix, ok);
    else
    {
        if(ok != 0)
            *ok = false;

        return 0;
    }
}


/*!
 * Given a raw (untokenized) mathematical expression in infix notation, create
 * the equalivalent postfix notation with spaces separating the tokens.
 * Allowable operators are " ( ) + - * / ^ ".
 * \param infix is the infix expresions to convert
 * \param ok is set to true if the conversions does not have problems. ok can point to NULL.
 * \return the equivalent postfix expression
 */
std::string ShuntingYard::infixToPostfix(const std::string& infix, bool* ok)
{
    bool Return = true;
    std::list<std::string> operatorStack;
    std::string postfix;

    // split the string by the separators
    std::vector<std::string> list = split(tokenize(infix), " ");

    for(std::size_t i = 0; i < list.size(); i++)
    {
        std::string o1 = list.at(i);

        if(o1.empty())
            continue;

        if(isNumber(o1))
        {
            postfix += o1;
            postfix += " ";
        }
        else if(o1.find('(') != std::string::npos)
            operatorStack.push_back(list.at(i));
        else if(o1.find(')') != std::string::npos)
        {
            std::string o2;

            // pop the stack until we hit the left paren. Notice neither
            // the left nor the right paren ends up in the output.
            while(operatorStack.size() > 0)
            {
                o2 = operatorStack.back();
                operatorStack.pop_back();

                if(o2.find('(') != std::string::npos)
                    break;
                else
                    postfix += o2 + " ";

            }// while operators to pop

            // If we never find the left paren then the input is malformed
            if(o2.find('(') == std::string::npos)
                Return = false;
        }
        else if(isOperator(o1))
        {
            while(operatorStack.size() > 0)
            {
                // o2 is not yet popped here
                std::string o2 = operatorStack.back();

                // Although this is the "operator" stack, o2 could be a parenthesis
                if(isOperator(o2))
                {
                    if(isLeftAssociative(o1) && (precedence(o1) <= precedence(o2)))
                    {
                        postfix += operatorStack.back() + " ";
                        operatorStack.pop_back();
                    }
                    else if(isRightAssociative(o1) && (precedence(o1) < precedence(o2)))
                    {
                        postfix += operatorStack.back() + " ";
                        operatorStack.pop_back();
                    }
                    else
                        break;
                }
                else
                    break;

            }// while operators to pop

            operatorStack.push_back(o1);

        }// if new operator
        else
        {
            // A character we don't recognize
            Return = false;
        }


    }// for all input tokens

    // Finally pop the remaining operators off the stack
    while(operatorStack.size() > 0)
    {
        std::string o2 = operatorStack.back();
        operatorStack.pop_back();

        // There can be no more parenthesis unless the input was bad
        if(isParen(o2.at(0)))
            Return = false;
        else
            postfix += o2 + " ";
    }

    if(ok != 0)
        *ok = Return;

    // Clean off any white space and return the result
    return trimm(postfix);

}// ShuntingYard::infixToPostfix


/*!
 * Given a postfix expression with spaces delimiting the tokens, compute the
 * result. computePostfix() is usually fed the output of infoxToPostfix().
 * Allowable operators are " + - * / ^ ".
 * \param postfix is the properly tokenzed postfix string
 * \param ok is set to true if the computation is god. ok can point to NULL.
 * \return the computational result, or 0 if the computation cannot be performed.
 */
double ShuntingYard::computePostfix(const std::string& postfix, bool* ok)
{
    std::list<double> arguments;

    // split the string by the separators
    std::vector<std::string> list = split(postfix, " ");

    for(std::size_t i = 0; i < list.size(); i++)
    {
        double arg;
        bool argok;
        std::string o1 = list.at(i);

        if(o1.empty())
            continue;

        // The argument is either an operator or a number
        arg = toNumber(o1, &argok);

        if(argok)
            arguments.push_back(arg);
        else if(arguments.size() >= 2)
        {
            // the rightmost argument is the top of the stack
            double arg2 = arguments.back();
            arguments.pop_back();

            double arg1 = arguments.back();
            arguments.pop_back();

            if(o1.find('^') != std::string::npos)
            {
                arguments.push_back(powf(arg1, arg2) != std::string::npos);
            }
            else if(o1.find('*') != std::string::npos)
            {
                arguments.push_back(arg1*arg2);
            }
            else if(o1.find('/') != std::string::npos)
            {
                arguments.push_back(arg1/arg2);
            }
            else if(o1.find('+') != std::string::npos)
            {
                arguments.push_back(arg1+arg2);
            }
            else if(o1.find('-') != std::string::npos)
            {
                arguments.push_back(arg1-arg2);
            }

        }// if token is operator
        else
        {
            // Something wrong here
            if(ok != 0)
                *ok = false;

            return 0;
        }

    }// for tokens

    // there should be one value left on the stack
    if(arguments.size() == 1)
    {
        // All is good
        if(ok != 0)
            *ok = true;

        return arguments.back();
    }
    else
    {
        // Something wrong here
        if(ok != 0)
            *ok = false;

        return 0;
    }

}// ShuntingYard::computePostfix


/*!
 * Return the operator precedence. Higher is greater precedence.
 * \param op is the operator to test
 * \return the precedence of the operator, from 4 (for exponentiation) to 2
 *         (for addition or subtraction). 0 if the operator is not recognized.
 */
int ShuntingYard::precedence(const std::string& op)
{
    if(op.find('^') != std::string::npos)
        return 4;
    else if(op.find('*') != std::string::npos)
        return 3;
    else if(op.find('/') != std::string::npos)
        return 3;
    else if(op.find('+') != std::string::npos)
        return 2;
    else if(op.find('-') != std::string::npos)
        return 2;
    else
        return 0;
}


/*!
 * \param op is the operator string to test
 * \return true if the operator is right associative
 */
bool ShuntingYard::isRightAssociative(const std::string& op)
{
    return !isLeftAssociative(op);
}


/*!
 * \param op is the operator string to test
 * \return true if the operator is left associative
 */
bool ShuntingYard::isLeftAssociative(const std::string& op)
{
    if(op.find('^') != std::string::npos)
        return false;
    else
        return true;
}


/*!
 * Convert an input string to an integer.
 * \param input is the string to convert. It can be a binary ("0b") or hex ("0x")
 *        or a decimal number.
 * \param ok will be set to true if the conversion is successful, else it will
 *         be set to false
 * \return the converted value, or 0 if the conversion fails
 */
int64_t ShuntingYard::toInt(std::string input, bool* ok)
{
    int64_t value = 0;
    int base = 10;
    std::string set;

    if(startsWith(input, "0b"))
    {
        base = 2;
        input.erase(0, 2);
        set = "01";
    }
    else if(startsWith(input, "0x"))
    {
        base = 16;
        input.erase(0, 2);
        set = "0123456789aAbBcCdDeEfF";
    }
    else
        set = "-0123456789";

    // Test if the number has any characters we do not support
    std::size_t index = input.find_first_not_of(set);

    if(index < input.size())
    {
        if(ok != nullptr)
            (*ok) = false;
    }
    else
    {
        if(ok != nullptr)
            (*ok) = true;

        try
        {
            value = (int64_t)std::stoll(input, nullptr, base);
        }
        catch (...)
        {
            value = 0;
            if(ok != nullptr)
                (*ok) = false;
        }
    }

    return value;

}// ShuntingYard::toInt


/*!
 * Convert an input string to an unsigned integer.
 * \param input is the string to convert. It can be a binary ("0b") or hex ("0x")
 *        or a decimal number.
 * \param ok will be set to true if the conversion is successful, else it will
 *         be set to false
 * \return the converted value, or 0 if the conversion fails
 */
uint64_t ShuntingYard::toUint(std::string input, bool* ok)
{
    uint64_t value = 0;
    int base = 10;
    std::string set;

    if(startsWith(input, "0b"))
    {
        base = 2;
        input.erase(0, 2);
        set = "01";
    }
    else if(startsWith(input, "0x"))
    {
        base = 16;
        input.erase(0, 2);
        set = "0123456789aAbBcCdDeEfF";
    }
    else
        set = "0123456789";

    // Test if the number has any characters we do not support
    std::size_t index = input.find_first_not_of(set);

    if(index < input.size())
    {
        if(ok != nullptr)
            (*ok) = false;
    }
    else
    {
        if(ok != nullptr)
            (*ok) = true;

        try
        {
            value = (uint64_t)std::stoull(input, nullptr, base);
        }
        catch (...)
        {
            value = 0;
            if(ok != nullptr)
                (*ok) = false;
        }
    }

    return value;

}// ShuntingYard::toUint


/*!
 * \param input is the string to test. It can be a binary ("0b") or hex ("0x")
 *        or an integer number
 * \return true if we can convert input to an integer
 */
bool ShuntingYard::isInt(const std::string& input)
{
    bool ok = false;

    toInt(input, &ok);

    return ok;
}


/*!
 * Convert an input string to a number.
 * \param input is the string to convert. It can be a binary ("0b") or hex ("0x")
 *        or a decimal number.
 * \param ok will be set to true if the conversion is successful, else it will
 *         be set to false
 * \return the converted value, or 0 if the conversion fails
 */
double ShuntingYard::toNumber(std::string input, bool* ok)
{
    double value = 0;

    if(startsWith(input, "0b") || startsWith(input, "0x"))
        return toInt(input, ok);

    // Test if the number has any characters we do not support
    std::size_t index = input.find_first_not_of("-.0123456789");

    if(index < input.size())
    {
        if(ok != nullptr)
            (*ok) = false;
    }
    else
    {
        if(ok != nullptr)
            (*ok) = true;

        try
        {
            value = std::stod(input);
        }
        catch (...)
        {
            value = 0;
            if(ok != nullptr)
                (*ok) = false;
        }
    }

    return value;

}// ShuntingYard::toNumber


/*!
 * \param input is the string to test. It can be a binary ("0b") or hex ("0x")
 *        or a decimal number
 * \return true if we can convert input to a number
 */
bool ShuntingYard::isNumber(const std::string& input)
{
    bool ok = false;

    toNumber(input, &ok);

    return ok;
}


/*!
 * \param input is the character to test
 * \param hex should be true to look for hexadecimal characters oly (not counting 'x')
 * \param binary should be true to look for binary characters only (not counting 'b')
 * \return true if the input is in the set ".0123456789" for decimal or "01"
 *         for binary or "0123456789aAbBcCdDeEfF" for hexadecimal.
 */
bool ShuntingYard::isNumber(char input, bool hex, bool binary)
{
    std::string set;

    if(binary)
        set = "01";
    else if(hex)
        set = "0123456789aAbBcCdDeEfF";
    else
        set = ".0123456789";

    return (set.find(input) != std::string::npos);
}


/*!
 * \param input is the string to test
 * \return true if the first character of the string is an operator from the set " + - * / ^ "
 */
bool ShuntingYard::isOperator(const std::string& input)
{
    return isOperator(input.at(0));
}


/*!
 * \param input is the character to test
 * \return true if the char is an operator from the set " + - * / ^ "
 */
bool ShuntingYard::isOperator(char input)
{
    return (std::string("+-*/^").find(input) != std::string::npos);
}


/*!
 * \param input is the character to test
 * \return true if the char is a parenthesis
 */
bool ShuntingYard::isParen(char input)
{
    if(input == '(')
        return true;
    else if(input == ')')
        return true;
    else
        return false;
}


/*!
 * Given an input string make sure that separators are applied between tokens
 * \param raw is the input string
 * \return the output string with space separators between tokens
 */
std::string ShuntingYard::tokenize(const std::string& raw)
{
    typedef enum
    {
        operation,
        number,
        numtokentypes
    }tokentypes;

    std::string output;
    std::string input = raw;
    tokentypes lasttoken = operation;
    char lastcharacter = ' ';

    // These flags track if we are parsing a binary or hex number, they can only be set by seeing the 0x or 0b prefix
    bool binary = false;
    bool hex = false;

    // Handle special numbers.
    replacePie(input);

    for(std::size_t i = 0; i < input.size(); i++)
    {
        char character = input.at(i);

        // this first case is hard, some numbers contain leading negative signs, but not hex or binary numbers.
        if(isNumber(character, hex, binary) || ((character == '-') && (lasttoken == operation) && !hex && !binary))
        {
            // Add a separator if the previous value was an operator. The goal is to keep numerals together
            if(lasttoken == operation)
                output += ' ';

            // remember the type
            lasttoken = number;

            // And output the character
            output += character;
        }
        else if(((character == 'x') || (character == 'X') || (character == 'b') || (character == 'B')) && (lastcharacter == '0'))
        {
            // Another hard case, we want to support hexadecimal and binary numbers,
            // but we need to preserve the "0x" or "0b" prefix

            // Treat this as a number, and note that last character must also have been a number
            lasttoken = number;

            // Output the character, no separator
            output += character;

            if((character == 'x') || (character == 'X'))
            {
                binary = false;
                hex = true;
            }
            else
            {
                binary = true;
                hex = false;
            }
        }
        else if(isOperator(character) || isParen(character))
        {
            // Whether following operator or number, add a separator
            output += ' ';

            // remember the type
            lasttoken = operation;

            // And output the character
            output += character;

            // Clear the binary and hex flags
            binary = false;
            hex = false;
        }
        else
        {
            // Add a separator
            output += ' ';

            // And output the character, this is going to be a failure
            output += character;

            // Clear the binary and hex flags
            binary = false;
            hex = false;
        }

        // Remember this for next pass
        lastcharacter = character;

    }// for all input characters

    return output;

}// ShuntingYard::tokenize


/*!
 * Test the shunting yard class
 * \return true if tests pass, else false
 */
bool ShuntingYard::test(void)
{
    bool ok;

    // Notice that in the test below the exponents are "stacked", and applied
    // from right to left, which means that (1-5) is raised to the 8th power
    double test = computeInfix("3+4*2/(1-5)^2^3");

    if(fabs(test - 3.0001220703125) > 0.0000000000001)
        return false;

    // Leading negatie is another strange one
    test = computeInfix("-3+4*2/(1-5)^2^3");
    if(fabs(test + 2.9998779296875) > 0.0000000000001)
        return false;

    test = computeInfix("300-262144/((1-5)^3)^3");
    if(fabs(test - 301) > 0.0000000000001)
        return false;

    // Negative exponential
    test = computeInfix("-300-1/((1-5)^3)^-3");
    if(fabs(test - 261844) > 0.0000000000001)
        return false;

    // A simple number
    test = computeInfix("-3.14159");
    if(fabs(test + 3.14159) > 0.0000000000001)
        return false;

    // Test pi
    test = computeInfix("360/(2*-pi)");
    if(fabs(test + 180.0/3.14159265358979323846) > 0.0000000000001)
        return false;

    // Test bad expression
    test = computeInfix("360/(2*-pi", &ok);
    if((fabs(test ) > 0.0000000000001) || (ok == true))
        return false;

    return true;
}
