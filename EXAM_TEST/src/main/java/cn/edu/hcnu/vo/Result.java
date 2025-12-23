import cn.edu.hcnu.po.Examinee;
import cn.edu.hcnu.service.ExamineeService;
import cn.edu.hcnu.vo.Result;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PatchMapping;

import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.ResponseBody;

import java.util.List;

@Controller
public class ExamineeController {

    @Autowired
    private ExamineeService examineeService;

    @GetMapping("/getAllExaminees")
    @ResponseBody
    public Result getAllExaminees() {
        List<Examinee> examineeList = examineeService.getExaminee();
        if (examineeList == null) {
            return new Result(500, "查询失败", examineeList);
        } else {
            return new Result(200, "查询成功", examineeList);
        }
    }

    @PostMapping("/deleteExaminee")
    @ResponseBody
    public Result deleteExaminee(Examinee examinee) {
        int i = examineeService.deleteExaminee(examinee.getId());
        if (i == 1) {
            return new Result(200, "删除成功", i);
        } else {
            return new Result(500, "删除失败", i);
        }
    }
}