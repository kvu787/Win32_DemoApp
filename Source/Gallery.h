#pragma once
#include <string>
#include <vector>
struct Page
{
    const wchar_t *title;
    const wchar_t *family;
    const wchar_t *identifiers;
    const wchar_t *explanation;
    const wchar_t *doc;
};
const std::vector<Page> &Pages();
struct InventoryItem
{
    const wchar_t *name;
    const wchar_t *cls;
    const wchar_t *coverage;
};
const std::vector<InventoryItem> &Inventory();
