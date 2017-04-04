#ifndef SHUNTINGYARD_H
#define SHUNTINGYARD_H

/*!
 * \file
 *
 * ShuntingYard implements the shunting yard algorithm originally conceived by
 * Edsger Dijkstra. The algorithm is used to convert an infix mathematical
 * expression to an equivalent postfix expression. The name "shunting yard"
 * refers to a common rail road track pattern used to shuffle the order of
 * railroad cars in a train. This algorithm uses a similar pattern with a
 * stack taking the place of the shunt track.
 *
 * ShuntingYard also provides the routine to compute the postfix expression.
 * Operators supported are: addition(+), subtraction(-), multiplication(*),
 * division(/), and exponentiation(^). Numerals must be input as decimal
 * numbers with or without decimal points. Hexadecimal, octal, binary, and
 * scientific notation are not allowed.
 *
 * In addition ShuntingYard understands the case insensitive strings "pi" and
 * "e", which are replaced with 3.14159265358979323846 and 2.71828182845904523536
 * respectively.
 *
 * \author Five By Five Development, LLC
 */

#include <QString>

class ShuntingYard
{
public:
    //! Compute an infix expresions
    static double computeInfix(const QString& infix, bool* ok = 0);

    //! Convert an infix expression toa properly delimited postfix expression
    static QString infixToPostfix(const QString& infix, bool* ok = 0);

    //! Compute a properly delimited postfix expression
    static double computePostfix(const QString& postfix, bool* ok = 0);

    //! Convert an input string to an integer
    static int toInt(QString input, bool* ok);

    //! Test if input string is an integer
    static bool isInt(const QString& input);

    //! Convert an input string to a number
    static double toNumber(QString input, bool* ok);

    //! Test if input string is a number
    static bool isNumber(const QString& input);

    //! Test if input character is a number
    static bool isNumber(const QChar& input);

    //! Replace "pi" or "e" in the string with the numeric values
    static QString& replacePie(QString& input);

    //! Test if input character is an operator
    static bool isOperator(const QChar& input);

    //! Test if input character is a parenthesis
    static bool isParen(const QChar& input);

    //! Test the ShuntingYard class
    static bool test();

private:

    //! Place delimiters as needed in the infix expression
    static QString tokenize(const QString& input);

    //! Return operator precedence
    static int precedence(const QString& op);

    //! Test if operator is right associative
    static bool isRightAssociative(const QString& op);

    //! Test if operator is left associative
    static bool isLeftAssociative(const QString& op);

    //! Test if input string is an operator
    static bool isOperator(const QString& input);

};

#endif // SHUNTINGYARD_H
