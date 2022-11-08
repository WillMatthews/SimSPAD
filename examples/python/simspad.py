from array import array


class SiPM:
    def __init__(self, *args, **kwargs):
        if len(args) == 1:
            self.dt = args[0][0]
            self.numMicrocell = args[0][1]
            self.vBias = args[0][2]
            self.vBr = args[0][3]
            self.tauRecovery = args[0][4]
            self.pdeMax1 = args[0][5]
            self.pdeMax2 = args[0][6]
            self.vChr = args[0][7]
            self.cCell = args[0][8]
            self.tauFwhm = args[0][9]
            self.digitalThreshold = args[0][10]
        else:
            self.dt = args[0]
            self.numMicrocell = args[1]
            self.vBias = args[2]
            self.vBr = args[3]
            self.tauRecovery = args[4]
            self.pdeMax1 = args[5]
            self.pdeMax2 = args[6]
            self.vChr = args[7]
            self.cCell = args[8]
            self.tauFwhm = args[9]
            self.digitalThreshold = args[10]

    def write_binary(self, filename, optical_input1, optical_input2):
        """simspad.SiPM.write_binary(filename, inputVector) creates a binary output file with the given file name,
        which packages the given input vector and SiPM parameters"""
        with open(filename, 'wb') as f:
            sipm_settings = [self.dt, self.numMicrocell, self.vBias, self.vBr,
                             self.tauRecovery, self.pdeMax1, self.pdeMax2, self.vChr, self.cCell, self.tauFwhm, self.digitalThreshold]

            optical_input = optical_input1 + optical_input2
            optical_input[::2] = optical_input1
            optical_input[1::2] = optical_input2

            double_array = array('d', sipm_settings + optical_input)
            double_array.tofile(f)

    def simulate_web(self, url, optical_input1, optical_input2):
        """simspad.SiPM.simulate_web(url, optical_input) sends a POST request to the SimSPAD Server application
        The SiPM's response is returned as a list"""
        import requests
        sipm_settings = [self.dt, self.numMicrocell, self.vBias, self.vBr,
                         self.tauRecovery, self.pdeMax1, self.pdeMax2, self.vChr, self.cCell, self.tauFwhm, self.digitalThreshold]

        optical_input = optical_input1 + optical_input2
        optical_input[::2] = optical_input1
        optical_input[1::2] = optical_input2

        binary_array = array('d', sipm_settings + optical_input)
        bytes_out = binary_array.tobytes()
        decoded_string = bytes_out.decode("ISO-8859-1")
        string_out = r'{}'.format(decoded_string)
        response = requests.post(url, data=string_out)
        char_response = response.text
        encoded_response = char_response.encode("ISO-8859-1")
        response = array('d', encoded_response)
        return response[10:-1].tolist()


def read_binary(filename):
    """simspad.read_binary(filename) reads a binary file with a given file name,
    and returns a tuple (vector, SiPM) whose first element is the associated vector,
    and the second element is the associated SiPM with parameters pre-configured"""
    with open(filename, 'rb') as f:
        numchar = len(f.read())
        numdouble = numchar//8
    with open(filename, 'rb') as f:
        binary_vals = array('d')
        binary_vals.fromfile(f, numdouble)

    settings_list = binary_vals[0:10]
    response = binary_vals[10:-1]

    return (response, SiPM(settings_list))
