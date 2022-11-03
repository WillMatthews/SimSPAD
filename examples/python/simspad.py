from array import array


class SiPM:
    def __init__(self, *args, **kwargs):
        if len(args) == 1:
            self.dt = args[0][0]
            self.numMicrocell = args[0][1]
            self.vBias = args[0][2]
            self.vBr = args[0][3]
            self.tauRecovery = args[0][4]
            self.pdeMax = args[0][5]
            self.vChr = args[0][6]
            self.cCell = args[0][7]
            self.tauFwhm = args[0][8]
            self.digitalThreshold = args[0][9]
        else:
            self.dt = args[0]
            self.numMicrocell = args[1]
            self.vBias = args[2]
            self.vBr = args[3]
            self.tauRecovery = args[4]
            self.pdeMax = args[5]
            self.vChr = args[6]
            self.cCell = args[7]
            self.tauFwhm = args[8]
            self.digitalThreshold = args[9]

    def write_binary(self, filename, optical_input):
        with open(filename, 'wb') as f:
            sipm_settings = [self.dt, self.numMicrocell, self.vBias, self.vBr,
                             self.tauRecovery, self.pdeMax, self.vChr, self.cCell, self.tauFwhm, self.digitalThreshold]
            double_array = array('d', sipm_settings + optical_input)
            double_array.tofile(f)

    def simulate_web(self, url, optical_input):
        import requests
        sipm_settings = [self.dt, self.numMicrocell, self.vBias, self.vBr,
                         self.tauRecovery, self.pdeMax, self.vChr, self.cCell, self.tauFwhm, self.digitalThreshold]
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
    with open(filename, 'rb') as f:
        numchar = len(f.read())
        numdouble = numchar//8
    with open(filename, 'rb') as f:
        binary_vals = array('d')
        binary_vals.fromfile(f, numdouble)

    settings_list = binary_vals[0:10]
    response = binary_vals[10:-1]

    return (response, SiPM(settings_list))
