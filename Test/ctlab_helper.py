import configparser


def read_configfile(cfg):
    cf = configparser.ConfigParser()
    cf.sections()
    cf.read(cfg)
    sp = (cf['DEFAULT']['Port'])
    uc = (cf['DEFAULT']['FileName'])
    return sp, uc

