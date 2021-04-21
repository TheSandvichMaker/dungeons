static bool
AreEqual(const String &a, const String &b)
{
    bool result = false;

    if (a.size == b.size)
    {
        result = true;
        for (size_t i = 0; i < a.size; ++i)
        {
            char c1 = a.data[i];
            char c2 = b.data[i];

            if (c1 != c2)
            {
                result = false;
                break;
            }
        }
    }

    return result;
}
