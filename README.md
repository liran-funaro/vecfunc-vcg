# vecfunc-vcg

VCG algorithms on `vecfunc` valuation functions.

# Features
- Implements the joint-func algorithm in C++ with a Python interface.
  * Supports multiple data structures options.
  * Supports multiple optimization flags.
- Implements Maille and Tuffin VCG Algorithm.


# Install (beta)
Install `g++-8`:
```bash
apt-get install g++-8 
```

Download and install [vecfunc](https://bitbucket.org/funaro/vecfunc) by cloning its repository and follow its `README.md` file to install it properly.

The Python library will compile the binary upon usage.
To precompile for all data types and for all dimensions, use the included script: `vecfuncvcglib/makeall.sh`.

See Python's requirements in the [REQUIREMENTS](REQUIREMENTS.txt) file.

Finally, install the package in developer mode:
```bash
python setup.py develop --user
```


# License
[GPL](LICENSE.txt)