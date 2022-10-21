'''
Original version: Lutz Raestatter Oct 1(?), 2021
Modify to work with flythrough: Oct 5, 2021 (Rebecca Ringuette)
'''
from datetime import datetime, timedelta, timezone

# standard model dictionary for reference
model_varnames = {'bx': ['B_x', 'x component of magnetic field',
                         0, 'GSE', 'car', ['time', 'x', 'y', 'z'], 'nT'],
                  'by': ['B_y', 'y component of magnetic field',
                         1, 'GSE', 'car', ['time', 'x', 'y', 'z'], 'nT'],
                  'bz': ['B_z', 'z component of magnetic field',
                         2, 'GSE', 'car', ['time', 'x', 'y', 'z'], 'nT'],
                  'ex': ['E_x', 'x component of electric field',
                         6, 'GSE', 'car', ['time', 'x', 'x', 'x'], 'mV/m'],
                  'ey': ['E_y', 'y component of electric field',
                         7, 'GSE', 'car', ['time', 'y', 'y', 'y'], 'mV/m'],
                  'ez': ['E_z', 'z component of electric field',
                         8, 'GSE', 'car', ['time', 'z', 'z', 'z'], 'mV/m'],
                  'vx': ['v_plasmax', 'x component of plasma velocity',
                         9, 'GSE', 'car', ['time', 'x', 'y', 'z'], 'km/s'],
                  'vy': ['v_plasmay', 'y component of plasma velocity',
                         10, 'GSE', 'car', ['time', 'x', 'y', 'z'], 'km/s'],
                  'vz': ['v_plasmaz', 'z component of plasma velocity',
                         11, 'GSE', 'car', ['time', 'x', 'y', 'z'], 'km/s'],
                  'rr': ['N_plasma', 'number density of plasma ' +
                         '(hydrogen equivalent)',
                         12, 'GSE', 'car', ['time', 'x', 'y', 'z'], '1/cm**3'],
                  'resis': ['eta', 'resistivity',
                            13, 'GSE', 'car', ['time', 'x', 'y', 'z'],
                            'm**2/s'],
                  'pp': ['P_plasma', 'plasma pressure',
                         14, 'GSE', 'car', ['time', 'x', 'y', 'z'], 'pPa'],
                  'xjx': ['j_x', 'current density, x component',
                          15, 'GSE', 'car', ['time', 'x', 'y', 'z'],
                          'muA/m**2'],
                  'xjy': ['j_y', 'current density, y component',
                          16, 'GSE', 'car', ['time', 'x', 'y', 'z'],
                          'muA/m**2'],
                  'xjz': ['j_z', 'current density, z component',
                          17, 'GSE', 'car', ['time', 'x', 'y', 'z'],
                          'muA/m**2'],
                  }
# 'bx1': ['B1_x', 'x component of magnetic field (on grid cell faces)',3,'GSE',
#       'car',['time', 'x', 'x', 'x'], 'nT'],
# 'by1': ['B1_y', 'y component of magnetic field (on grid cell faces)',4,'GSE',
#       'car',['time', 'y', 'y', 'y'], 'nT'],
# 'bz1': ['B1_z', 'z component of magnetic field (on grid cell faces)',5,'GSE',
#       'car',['time', 'z', 'z', 'z'], 'nT'],
# linkage to grid position vectors are established during variable registration
# these are gx_bx, gy_bx, ... gz_ez affecting magnetic field (b1x,b1y,b1z) and
#       electric field (ex,ey,ez)


def ts_to_hrs(time_val, filedate):
    '''Convert utc timestamp to hours since midnight on filedate.'''

    return (datetime.utcfromtimestamp(time_val).replace(tzinfo=timezone.utc) -
            filedate).total_seconds()/3600.


def hrs_to_ts(hrs, filedate):
    '''Add hours to filedate and return utc timestamp.'''

    return datetime.timestamp(filedate+timedelta(hours=float(hrs)))


def MODEL():
    from numpy import array, NaN, diff
    from time import perf_counter
    import psutil
    from os.path import isfile, getsize
    from netCDF4 import Dataset
    from kamodo import Kamodo
    import kamodo_ccmc.readers.reader_utilities as RU

    class MODEL(Kamodo):
        '''OpenGGCM_GM magnetosphere reader.

        Inputs:
            full_file_prefix: a string representing the file pattern of the model
                output data.
                Note: This reader takes a file pattern of the format
                file_dir+runname+'.3df_YYYY-MM-DD_HH', where file_dir is the complete
                file path to the data files, runname is the name of the run
                embedded in the file name, and YYYY-MM-DD_HH is the four digit
                year, two digit month, two digit day, and two digit hour,
                assuming a 24 hour convention, in the desired output file names
                (e.g. 2015-03-15_13 for March 15, 2015 at 1 pm UTC).
            variables_requested = a list of variable name strings chosen from the
                model_varnames dictionary in this script, specifically the first
                item in the list associated with a given key.
                - If empty, the reader functionalizes all possible variables (default)
                - If 'all', the reader returns the model_varnames dictionary above
                    for only the variables present in the given files. Note: the
                    fulltime keyword must be False to acheive this behavior.
            filetime = boolean (default = False)
                - if False, the script fully executes.
                - If True, the script only executes far enough to determine the
                    time values associated with the chosen data.
                Note: The behavior of the script is determined jointly by the
                    filetime and fulltime keyword values.
            printfiles = boolean (default = False)
                - If False, the filenames associated with the data retrieved ARE
                    NOT printed.
                - If True, the filenames associated with the data retrieved ARE
                    printed.
            gridded_int = boolean (default = True)
                - If True, the variables chosen are functionalized in both the
                    standard method and a gridded method.
                - If False, the variables chosen are functionalized in only the
                    standard method.
            fulltime = boolean (default = True)
                - If True, linear interpolation in time between files is included
                    in the returned interpolator functions.
                - If False, no linear interpolation in time between files is included.
            verbose = boolean (False)
                - If False, script execution and the underlying Kamodo execution
                    is quiet except for specified messages.
                - If True, be prepared for a plethora of messages.
        All inputs are described in further detail in KamodoOnboardingInstructions.pdf.

        Returns: a kamodo object (see Kamodo core documentation) containing all
            requested variables in functionalized form.
        '''
        def __init__(self, full_file_prefix, variables_requested=[],
                     filetime=False, verbose=False, gridded_int=True,
                     printfiles=False, fulltime=True, missing_value=NaN,
                     **kwargs):
            super(MODEL, self).__init__()
            self.modelname = 'OpenGGCM_GM'
            t0 = perf_counter()  # profiling time stamp

            # convert files to netcdf4 if needed
            # input file name: file_dir/YYYY-MM-DD_HH.nc
            nc_file = full_file_prefix+'.nc'
            if isfile(nc_file):  # file already prepared!
                self.conversion_test = True  # default value
            else:  # file not prepared, prepare it
                try:  # I don't have the file converter, so leave in try/except
                    from kamodo_ccmc.readers.openggcm_to_cdf import \
                        openggcm_combine_magnetosphere_files as gmconv
                    self.conversion_test = gmconv(full_file_prefix)
                    # should return a boolean (True if success, False if not)
                except:
                    self.conversion_test = False
            self._file = nc_file

            # data are time-wrapped in files.
            cdf_data = Dataset(nc_file)
            self._time = array(cdf_data.variables['_time'])  # hrs since 12am

            # establish time attributes first
            self.filedate = datetime.strptime(cdf_data.filedate+' 00:00:00',
                                              '%Y-%m-%d %H:%M:%S').replace(
                                                  tzinfo=timezone.utc)
            if len(self._time) > 1:
                self.dt = diff(self._time).max()*3600.  # hours -> seconds
            else:
                self.dt = 0
            self.datetimes = [datetime.utcfromtimestamp(
                hrs_to_ts(self._time[0], self.filedate)).isoformat(sep=' '),
                              datetime.utcfromtimestamp(
                                  hrs_to_ts(self._time[-1],
                                            self.filedate)).isoformat(sep=' ')]
            # store beg and end UTC timestamps
            self.filetimes = [hrs_to_ts(self._time[0], self.filedate),
                              hrs_to_ts(self._time[-1], self.filedate)]

            # return time information only for flythrough
            if filetime:
                return

            # if variables are given as integers, convert to standard names
            if len(variables_requested) > 0:
                if isinstance(variables_requested[0], int):
                    print('Integers detected. Converting...', end="")
                    tmp_var = [value[0] for key, value in
                               model_varnames.items()
                               if value[2] in variables_requested]
                    variables_requested = tmp_var
                    print('Converted:', variables_requested)

            # perform initial check on variables_requested list
            if len(variables_requested) > 0 and fulltime and \
                    variables_requested != 'all':
                test_list = [value[0] for key, value in model_varnames.items()]
                err_list = [item for item in variables_requested if item not in
                            test_list]
                if len(err_list) > 0:
                    print('Variable name(s) not recognized:', err_list)

            # collect variable list
            if len(variables_requested) > 0 and variables_requested != 'all':
                gvar_list = [key for key, value in model_varnames.items()
                             if value[0] in variables_requested and
                             key in cdf_data.variables.keys()]  # file v names

                # check for variables requested but not available
                if len(gvar_list) != len(variables_requested):
                    err_list = [value[0] for key, value in
                                model_varnames.items()
                                if value[0] in variables_requested and
                                key not in cdf_data.variables.keys()]
                    if len(err_list) > 0:
                        print('Some requested variables are not available:',
                              err_list)
            else:  # only input variables on the avoid_list if requested
                avoid_list = []   # empty for now
                gvar_list = [key for key in cdf_data.variables.keys()
                             if key in model_varnames.keys() and
                             key not in avoid_list]
                if not fulltime and variables_requested == 'all':
                    self.var_dict = {value[0]: value[1:] for key, value in
                                     model_varnames.items() if key in
                                     gvar_list}
                    return

            # Store variable's units and reference to Dataset object in memory
            variables = {model_varnames[key][0]: 
                         {'units': model_varnames[key][-1],
                          'data': cdf_data.variables[key]}
                         for key in gvar_list}
            if verbose:
                print('Done reading in variable data.', full_file_prefix)

            # store variables
            self.near_Earth_boundary_radius = \
                cdf_data.near_Earth_boundary_radius
            self.near_Earth_boundary_radius_unit = \
                cdf_data.near_Earth_boundary_radius_units
            self.missing_value = NaN
            self.verbose = verbose
            self.filename = cdf_data.file.split(',')
            self.modelname = cdf_data.model
            self.modelname = 'OpenGGCM_GM'
            self._registered = 0
            if printfiles:
                print('Files:')
                for file in self.filename:
                    print(file)

            # add coordinate grids as needed
            # grid_list = list of coordinate names in cdf file
            grid_list = ['_x', '_y', '_z', '_x_bx', '_y_bx', '_z_bx', '_x_by',
                         '_y_by', '_z_by', '_x_bz', '_y_bz', '_z_bz', '_x_ex',
                         '_y_ex', '_z_ex', '_x_ey', '_y_ey', '_z_ey', '_x_ez',
                         '_y_ez', '_z_ez']
            # trim down to only save coordinate grids needed for variables
            #  requested and available in file
            if 'B1_x' not in variables.keys():
                grid_list.remove('_x_bx')
                grid_list.remove('_y_bx')
                grid_list.remove('_z_bx')
            if 'B1_y' not in variables.keys():
                grid_list.remove('_x_by')
                grid_list.remove('_y_by')
                grid_list.remove('_z_by')
            if 'B1_z' not in variables.keys():
                grid_list.remove('_x_bz')
                grid_list.remove('_y_bz')
                grid_list.remove('_z_bz')
            if 'E1_x' not in variables.keys():
                grid_list.remove('_x_ex')
                grid_list.remove('_y_ex')
                grid_list.remove('_z_ex')
            if 'E1_y' not in variables.keys():
                grid_list.remove('_x_ey')
                grid_list.remove('_y_ey')
                grid_list.remove('_z_ey')
            if 'E1_z' not in variables.keys():
                grid_list.remove('_x_ez')
                grid_list.remove('_y_ez')
                grid_list.remove('_z_ez')
            for grid in grid_list:
                # store coordinate data
                setattr(self, grid, getattr(cdf_data, grid).values)
            # leave cdf file open

            # register interpolators for each variable
            varname_list, self.variables = [key for key in variables.keys()], {}
            t_reg = perf_counter()
            for varname in varname_list:  # all are 3D variables
                # store and register data
                self.variables[varname] = {
                    'units': variables[varname]['units'],
                    'data': variables[varname]['data']}
                self.register_variable(varname, gridded_int)
            if verbose:
                print(f'Took {perf_counter()-t_reg:.5f}s to register ' +
                      f'{len(varname_list)} variables.')
            if verbose:
                print(f'Took a total of {perf_counter()-t0:.5f}s to kamodofy ' +
                      f'{len(gvar_list)} variables.')


        def get_grid(self, varname):
            """fetch the grid positon for this variable"""

            if varname == 'B1_x':
                return self._x_bx, self._y_bx, self._z_bx
            elif varname == 'B1_y':
                return self._x_by, self._y_by, self._z_by
            elif varname == 'B1_z':
                return self._x_bz, self._y_bz, self._z_bz
            elif varname == 'E1_x':
                return self._x_ex, self._y_ex, self._z_ex
            elif varname == 'E1_y':
                return self._x_ey, self._y_ey, self._z_ey
            elif varname == 'E1_z':
                return self._x_ez, self._y_ez, self._z_ez
            else:  # (default) positions on plasma grid
                return self._x, self._y, self._z


        # define and register a 4D variable (all are 4D)
        def register_variable(self, varname, gridded_int):
            '''Register and functionalize the variable data.'''
            x_, y_, z_ = self.get_grid(varname)
            
            coord_dict = {'time': {'data': self._time, 'units': 'hr'},
                          'X': {'data': x_, 'units': 'R_E'},
                          'Y': {'data': y_, 'units': 'R_E'},
                          'Z': {'data': z_, 'units': 'R_E'}}
            coord_str = [value[3]+value[4] for key, value in
                         model_varnames.items() if value[0] == varname][0]
            if (getsize(self._file) > psutil.virtual_memory().available):
                self = RU.time_interp(self, coord_dict, varname,
                                      self.variables[varname], gridded_int,
                                      coord_str)
                print('File is larger than available memory. ' +
                      f'Using lazy interpolation for {varname}.')
            else:  # if enough memory, go for it
                self.variables[varname]['data'] = array(
                    self.variables[varname]['data'])
                self = RU.Functionalize_Dataset(self, coord_dict, varname,
                                                self.variables[varname],
                                                gridded_int, coord_str)
            return
    return MODEL
