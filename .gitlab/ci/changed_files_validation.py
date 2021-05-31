#!/usr/bin/env python3.8

# (c) 2021 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
# file or any portion thereof may not be reproduced or used in any manner
# whatsoever without the express written permission of KAI OS TECHNOLOGIES
# (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
# KONG) LIMITED or its affiliate company and may be registered in some
# jurisdictions. All other trademarks are the property of their respective
# owners.
#

import sys
from pathlib import Path
from typing import Iterable

cwd = Path.cwd()
if not cwd.joinpath('.gitlab-ci.yml').exists():
    print(f"Current working directory: {cwd}, should be \"gecko\".")
    sys.exit(-1)

changed_files = sys.argv[1:]
if not changed_files:
    print("changed_files is empty.")
    sys.exit(-1)


class ChangedFilesValidator:
    error_msg = '{invalid} is not supposed to be applied to production branch due to external dependency.'

    def __init__(self, invalid_files: Iterable) -> None:
        self.invalid_files = invalid_files

    def __str__(self) -> str:
        return f"[{self.__class__.__name__}]: {self.__doc__}"

    def __call__(self) -> bool:
        print(self)

        def filter_out_invalid(iterable) -> bool:
            if invalid := next((cf for cf in changed_files if cf in iterable), None):
                print(self.__class__.error_msg.format(invalid=invalid))
                return False
            return True

        if isinstance(self.invalid_files, str):
            if not (iff := cwd.joinpath(self.invalid_files)).exists():
                print(f"{iff} not exist !!")
                return False
            return filter_out_invalid(iff.read_text().split('\n'))

        return filter_out_invalid(self.invalid_files)


class SidlServicesValidator(ChangedFilesValidator):
    """Check if MR contains changes that depend on services defined in SIDL."""
    sidl_services_list_file = '.gitlab/ci/scm/sidl_services'

    def __init__(self) -> None:
        super().__init__(self.__class__.sidl_services_list_file)


if __name__ == "__main__":
    if any(not cfv()() for cfv in ChangedFilesValidator.__subclasses__()):
        sys.exit(-1)
