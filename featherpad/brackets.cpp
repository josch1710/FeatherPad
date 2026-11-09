/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2014-2021 <tsujan2000@gmail.com>
 *
 * FeatherPad is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FeatherPad is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <https://spdx.org/licenses/GPL-3.0+.html>
 */

#include "fpwin.h"
#include "ui_fp.h"

namespace FeatherPad {

void FPwin::matchBrackets()
{
    TabPage *tabPage = qobject_cast< TabPage *>(ui->tabWidget->currentWidget());
    if (tabPage == nullptr) return;
    TextEdit *textEdit = tabPage->textEdit();
    QTextCursor cur = textEdit->textCursor();
    TextBlockData *data = static_cast<TextBlockData *>(cur.block().userData());
    if (!data) return;

    textEdit->matchedBrackets();

    QList<QTextEdit::ExtraSelection> es = textEdit->extraSelections();
    int n = textEdit->getRedSel().count();
    while (n > 0
           && !es.isEmpty()) // just a precaution against crash
    {
        es.removeLast();
        --n;
    }
    textEdit->setRedSel (QList<QTextEdit::ExtraSelection>());
    textEdit->setExtraSelections (es);

    QTextDocument *doc = textEdit->document();
    int curPos = cur.position();
    /* position of block's first character */
    int blockPos = cur.block().position();
    /* position of cursor in block */
    int curBlockPos = curPos - blockPos;

    /* parenthesis */
    bool isAtLeft (doc->characterAt (curPos) == '(');
    bool isAtRight (doc->characterAt (curPos - 1) == ')');
    bool findNextBrace (!isAtLeft || !isAtRight);
    if (isAtLeft || isAtRight)
    {
        QList<ParenthesisInfo *> infos = data->parentheses();
        for (int i = 0; i < infos.size(); ++i)
        {
            ParenthesisInfo *info = infos.at (i);

            if (isAtLeft && info->position == curBlockPos && info->character == '(')
            {
                if (matchLeftParenthesis (cur.block(), i + 1, 0))
                {
                    createSelection (blockPos + info->position);
                    if (isAtRight) isAtLeft = false;
                    else break;
                }
            }
            if (isAtRight && info->position == curBlockPos - 1 && info->character == ')')
            {
                if (matchRightParenthesis (cur.block(), infos.size() - i, 0))
                {
                    createSelection (blockPos + info->position);
                    if (isAtLeft) isAtRight = false;
                    else break;
                }
            }
        }
    }
    if (!findNextBrace) return;

    /* brace */
    isAtLeft = (doc->characterAt (curPos) == '{');
    isAtRight = (doc->characterAt (curPos - 1) == '}');
    findNextBrace = !isAtLeft || !isAtRight;
    if (isAtLeft || isAtRight)
    {
        QList<BraceInfo *> braceInfos = data->braces();
        for (int i = 0; i < braceInfos.size(); ++i)
        {
            BraceInfo *info = braceInfos.at (i);

            if (isAtLeft && info->position == curBlockPos && info->character == '{')
            {
                if (matchLeftBrace (cur.block(), i + 1, 0))
                {
                    createSelection (blockPos + info->position);
                    if (isAtRight) isAtLeft = false;
                    else break;
                }
            }
            if (isAtRight && info->position == curBlockPos - 1 && info->character == '}')
            {
                if (matchRightBrace (cur.block(), braceInfos.size() - i, 0))
                {
                    createSelection (blockPos + info->position);
                    if (isAtLeft) isAtRight = false;
                    else break;
                }
            }
        }
    }
    if (!findNextBrace) return;

    /* bracket */
    isAtLeft = (doc->characterAt (curPos) == '[');
    isAtRight = (doc->characterAt (curPos - 1) == ']');
    if (isAtLeft || isAtRight)
    {
        QList<BracketInfo *> bracketInfos = data->brackets();
        for (int i = 0; i < bracketInfos.size(); ++i)
        {
            BracketInfo *info = bracketInfos.at (i);

            if (isAtLeft && info->position == curBlockPos && info->character == '[')
            {
                if (matchLeftBracket (cur.block(), i + 1, 0))
                {
                    createSelection (blockPos + info->position);
                    if (isAtRight) isAtLeft = false;
                    else break;
                }
            }
            if (isAtRight && info->position == curBlockPos - 1 && info->character == ']')
            {
                if (matchRightBracket (cur.block(), bracketInfos.size() - i, 0))
                {
                    createSelection (blockPos + info->position);
                    if (isAtLeft) isAtRight = false;
                    else break;
                }
            }
        }
    }
}
/*******************************************************************************
 Instead of making the following match functions recursive and setting a limit
 on their recursion depth, for preventing rare stack overflows with huge files,
 loops have been added to them. This method works with exceptionally huge files,
 without any need to a built-in limit.
 *******************************************************************************/
bool FPwin::matchLeftParenthesis (QTextBlock currentBlock, int i, int numLeftParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    if (!data) return false;
    QList<ParenthesisInfo *> infos = data->parentheses();
    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        ParenthesisInfo *info = infos.at (i);
        if (info->character == '(')
        {
            ++numLeftParentheses;
            continue;
        }
        if (info->character == ')' && numLeftParentheses == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numLeftParentheses;
    }
    currentBlock = currentBlock.next();

    while (currentBlock.isValid())
    {
        data = static_cast<TextBlockData *>(currentBlock.userData());
        if (!data) return false;
        QList<ParenthesisInfo *> infos = data->parentheses();
        i = 0;
        int docPos = currentBlock.position();
        for (; i < infos.size(); ++i)
        {
            ParenthesisInfo *info = infos.at (i);
            if (info->character == '(')
            {
                ++numLeftParentheses;
                continue;
            }
            if (info->character == ')' && numLeftParentheses == 0)
            {
                createSelection (docPos + info->position);
                return true;
            }
            else
                --numLeftParentheses;
        }
        currentBlock = currentBlock.next();
    }

    return false;
}
/*************************/
bool FPwin::matchRightParenthesis (QTextBlock currentBlock, int i, int numRightParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    if (!data) return false;
    QList<ParenthesisInfo *> infos = data->parentheses();
    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        ParenthesisInfo *info = infos.at (infos.size() - 1 - i);
        if (info->character == ')')
        {
            ++numRightParentheses;
            continue;
        }
        if (info->character == '(' && numRightParentheses == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numRightParentheses;
    }
    currentBlock = currentBlock.previous();

    while (currentBlock.isValid())
    {
        data = static_cast<TextBlockData *>(currentBlock.userData());
        if (!data) return false;
        QList<ParenthesisInfo *> infos = data->parentheses();
        i = 0;
        int docPos = currentBlock.position();
        for (; i < infos.size(); ++i)
        {
            ParenthesisInfo *info = infos.at (infos.size() - 1 - i);
            if (info->character == ')')
            {
                ++numRightParentheses;
                continue;
            }
            if (info->character == '(' && numRightParentheses == 0)
            {
                createSelection (docPos + info->position);
                return true;
            }
            else
                --numRightParentheses;
        }
        currentBlock = currentBlock.previous();
    }

    return false;
}
/*************************/
bool FPwin::matchLeftBrace (QTextBlock currentBlock, int i, int numRightBraces)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    if (!data) return false;
    QList<BraceInfo *> infos = data->braces();
    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        BraceInfo *info = infos.at (i);
        if (info->character == '{')
        {
            ++numRightBraces;
            continue;
        }
        if (info->character == '}' && numRightBraces == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numRightBraces;
    }
    currentBlock = currentBlock.next();

    while (currentBlock.isValid())
    {
        data = static_cast<TextBlockData *>(currentBlock.userData());
        if (!data) return false;
        QList<BraceInfo *> infos = data->braces();
        i = 0;
        docPos = currentBlock.position();
        for (; i < infos.size(); ++i)
        {
            BraceInfo *info = infos.at (i);
            if (info->character == '{')
            {
                ++numRightBraces;
                continue;
            }
            if (info->character == '}' && numRightBraces == 0)
            {
                createSelection (docPos + info->position);
                return true;
            }
            else
                --numRightBraces;
        }
        currentBlock = currentBlock.next();
    }

    return false;
}
/*************************/
bool FPwin::matchRightBrace (QTextBlock currentBlock, int i, int numLeftBraces)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    if (!data) return false;
    QList<BraceInfo *> infos = data->braces();
    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        BraceInfo *info = infos.at (infos.size() - 1 - i);
        if (info->character == '}')
        {
            ++numLeftBraces;
            continue;
        }
        if (info->character == '{' && numLeftBraces == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numLeftBraces;
    }
    currentBlock = currentBlock.previous();

    while (currentBlock.isValid())
    {
        data = static_cast<TextBlockData *>(currentBlock.userData());
        if (!data) return false;
        QList<BraceInfo *> infos = data->braces();
        i = 0;
        docPos = currentBlock.position();
        for (; i < infos.size(); ++i)
        {
            BraceInfo *info = infos.at (infos.size() - 1 - i);
            if (info->character == '}')
            {
                ++numLeftBraces;
                continue;
            }
            if (info->character == '{' && numLeftBraces == 0)
            {
                createSelection (docPos + info->position);
                return true;
            }
            else
                --numLeftBraces;
        }
        currentBlock = currentBlock.previous();
    }

    return false;
}
/*************************/
bool FPwin::matchLeftBracket (QTextBlock currentBlock, int i, int numRightBrackets)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    if (!data) return false;
    QList<BracketInfo *> infos = data->brackets();
    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        BracketInfo *info = infos.at (i);
        if (info->character == '[')
        {
            ++numRightBrackets;
            continue;
        }
        if (info->character == ']' && numRightBrackets == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numRightBrackets;
    }
    currentBlock = currentBlock.next();

    while (currentBlock.isValid())
    {
        data = static_cast<TextBlockData *>(currentBlock.userData());
        if (!data) return false;
        QList<BracketInfo *> infos = data->brackets();
        i = 0;
        int docPos = currentBlock.position();
        for (; i < infos.size(); ++i)
        {
            BracketInfo *info = infos.at (i);
            if (info->character == '[')
            {
                ++numRightBrackets;
                continue;
            }
            if (info->character == ']' && numRightBrackets == 0)
            {
                createSelection (docPos + info->position);
                return true;
            }
            else
                --numRightBrackets;
        }
        currentBlock = currentBlock.next();
    }

    return false;
}
/*************************/
bool FPwin::matchRightBracket (QTextBlock currentBlock, int i, int numLeftBrackets)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    if (!data) return false;
    QList<BracketInfo *> infos = data->brackets();
    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i)
    {
        BracketInfo *info = infos.at (infos.size() - 1 - i);
        if (info->character == ']')
        {
            ++numLeftBrackets;
            continue;
        }
        if (info->character == '[' && numLeftBrackets == 0)
        {
            createSelection (docPos + info->position);
            return true;
        }
        else
            --numLeftBrackets;
    }
    currentBlock = currentBlock.previous();

    while (currentBlock.isValid())
    {
        data = static_cast<TextBlockData *>(currentBlock.userData());
        if (!data) return false;
        QList<BracketInfo *> infos = data->brackets();
        i = 0;
        int docPos = currentBlock.position();
        for (; i < infos.size(); ++i)
        {
            BracketInfo *info = infos.at (infos.size() - 1 - i);
            if (info->character == ']')
            {
                ++numLeftBrackets;
                continue;
            }
            if (info->character == '[' && numLeftBrackets == 0)
            {
                createSelection (docPos + info->position);
                return true;
            }
            else
                --numLeftBrackets;
        }
        currentBlock = currentBlock.previous();
    }

    return false;
}
/*************************/
void FPwin::createSelection (int pos)
{
    TabPage *tabPage = qobject_cast< TabPage *>(ui->tabWidget->currentWidget());
    if (tabPage == nullptr) return;
    TextEdit *textEdit = tabPage->textEdit();

    QList<QTextEdit::ExtraSelection> es = textEdit->extraSelections();

    QTextCursor cursor = textEdit->textCursor();
    cursor.setPosition (pos);
    cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

    QTextEdit::ExtraSelection extra;
    extra.format.setBackground (textEdit->hasDarkScheme() ? QColor (190, 0, 3) : QColor (255, 150, 150));
    extra.cursor = cursor;

    QList<QTextEdit::ExtraSelection> rsel = textEdit->getRedSel();
    rsel.append (extra);
    textEdit->setRedSel (rsel);
    es.append (extra);

    textEdit->setExtraSelections (es);
}

}
