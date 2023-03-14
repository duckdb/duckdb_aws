import duckdb

def test_aws():
    conn = duckdb.connect('');
    conn.execute("SELECT aws('Sam') as value;");
    res = conn.fetchall()
    assert(res[0][0] == "Aws Sam 🐥");